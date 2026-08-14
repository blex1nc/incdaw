// Phase 11 — one generic automation subsystem.
//
// The roadmap's exit criterion is the load-bearing test: any parameter
// registered in the parameter system is automatable with no parameter-specific
// code. The test for it registers a key the codebase has never heard of and
// automates it through the compiled graph — if that passes, "volume" and "pan"
// are just two more registrations.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/AutomationCommands.h"
#include "engine/automation/AutomationSequence.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/transport/TempoMap.h"
#include "project/ParameterRegistry.h"
#include "project/ProjectGraphCompiler.h"

#include <atomic>
#include <memory>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

engine::AutomationPoint enginePoint(Tick tick, float value,
                                    engine::AutomationShape shape = engine::AutomationShape::linear,
                                    float tension = 0.0f)
{
    return {tick, value, shape, tension};
}

project::AutomationPoint modelPoint(Tick tick, double value,
                                    project::AutomationCurve curve = project::AutomationCurve::linear)
{
    return {tick, value, curve, 0.0};
}

/// A project with one audible channel, ready to compile.
struct AutomationFixture {
    project::Project      project;
    project::EntityId     channel;
    project::EntityId     pattern;
    engine::TempoMap      tempo;

    AutomationFixture()
    {
        channel = project.addChannel("Channel 1").id;
        pattern = project.addPattern("Pattern 1").id;

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.duration = 240;
        project.findPattern(pattern)->contentFor(channel).events.push_back(note);

        tempo.setSampleRate(48000.0);
    }

    [[nodiscard]] project::CompiledProjectGraph
    compile(const project::ParameterRegistry* registry = nullptr)
    {
        project::GraphCompileOptions options;
        options.pattern    = pattern;
        options.parameters = registry;
        return project::compileProjectGraph(project, tempo, options);
    }

    /// Renders one block at `frame`, which is what makes the AutomationNode run.
    static void renderAt(project::CompiledProjectGraph& compiled, engine::FramePosition frame)
    {
        engine::AudioBufferPool pool;
        pool.allocate(1, 2, 64);
        pool.buffer(0).clear();
        compiled.graph->process(pool.buffer(0), 64, frame);
    }
};

} // namespace

// ── Curve arithmetic ──────────────────────────────────────────────────────────

TEST_CASE("an automation sequence evaluates its segment shapes")
{
    engine::AutomationSequence sequence;
    sequence.setPoints({enginePoint(0, 0.0f), enginePoint(1000, 1.0f)});

    // An envelope, not a loop: it holds outside its ends.
    CHECK(sequence.valueAt(-50) == doctest::Approx(0.0));
    CHECK(sequence.valueAt(2000) == doctest::Approx(1.0));

    CHECK(sequence.valueAt(500) == doctest::Approx(0.5));
    CHECK(sequence.valueAt(250) == doctest::Approx(0.25));

    SUBCASE("hold jumps at the next point")
    {
        sequence.setPoints({enginePoint(0, 0.2f, engine::AutomationShape::hold),
                            enginePoint(1000, 0.9f)});

        CHECK(sequence.valueAt(999) == doctest::Approx(0.2));
        CHECK(sequence.valueAt(1000) == doctest::Approx(0.9));
    }

    SUBCASE("smooth has zero slope at both ends")
    {
        sequence.setPoints({enginePoint(0, 0.0f, engine::AutomationShape::smooth),
                            enginePoint(1000, 1.0f)});

        CHECK(sequence.valueAt(500) == doctest::Approx(0.5));

        // Near the ends the smooth curve moves less than the straight line.
        CHECK(sequence.valueAt(100) < 0.1f);
        CHECK(sequence.valueAt(900) > 0.9f);
    }

    SUBCASE("exponential starts slow")
    {
        sequence.setPoints({enginePoint(0, 0.0f, engine::AutomationShape::exponential),
                            enginePoint(1000, 1.0f)});

        CHECK(sequence.valueAt(500) == doctest::Approx(0.25));
    }

    SUBCASE("tension bends the segment without moving its ends")
    {
        sequence.setPoints({enginePoint(0, 0.0f, engine::AutomationShape::linear, 0.8f),
                            enginePoint(1000, 1.0f)});

        CHECK(sequence.valueAt(0) == doctest::Approx(0.0));
        CHECK(sequence.valueAt(1000) == doctest::Approx(1.0));
        CHECK(sequence.valueAt(500) < 0.5f);   // bent toward the start

        sequence.setPoints({enginePoint(0, 0.0f, engine::AutomationShape::linear, -0.8f),
                            enginePoint(1000, 1.0f)});
        CHECK(sequence.valueAt(500) > 0.5f);   // bent toward the end
    }

    SUBCASE("unsorted input is sorted at set time")
    {
        sequence.setPoints({enginePoint(1000, 1.0f), enginePoint(0, 0.0f)});
        CHECK(sequence.valueAt(500) == doctest::Approx(0.5));
    }
}

// ── The exit criterion ────────────────────────────────────────────────────────

TEST_CASE("a freshly registered parameter is automatable with no parameter-specific code")
{
    AutomationFixture fixture;

    // A parameter this codebase has never heard of. Registration is ALL that
    // happens; if the value below arrives, nothing anywhere needed to know the
    // key existed.
    auto received = std::make_shared<std::atomic<float>>(-1.0f);

    project::ParameterRegistry registry = project::ParameterRegistry::withBuiltins();
    registry.registerParameter("acme.wobble",
                               [received](engine::dsp::MixerStripNode&, float value) {
                                   received->store(value, std::memory_order_relaxed);
                               });

    project::AutomationLane& lane =
        fixture.project.addAutomationLane(fixture.channel, "acme.wobble");
    lane.points.push_back(modelPoint(0, 0.0));
    lane.points.push_back(modelPoint(ticksPerQuarterNote * 4, 1.0));

    auto compiled = fixture.compile(&registry);
    REQUIRE(compiled);
    REQUIRE(compiled.automation != nullptr);
    CHECK(compiled.automation->bindingCount() == 1);

    // Halfway through the four-beat ramp: two beats, at 120 bpm and 48 kHz.
    const engine::FramePosition halfway = fixture.tempo.frameForTick(ticksPerQuarterNote * 2);
    AutomationFixture::renderAt(compiled, halfway);

    CHECK(received->load(std::memory_order_relaxed) == doctest::Approx(0.5).epsilon(0.02));
}

TEST_CASE("a volume lane drives the mixer strip through the compiled graph")
{
    AutomationFixture fixture;

    project::AutomationLane& lane =
        fixture.project.addAutomationLane(fixture.project.masterMixerNode(), "volume");
    lane.points.push_back(modelPoint(0, 0.0));
    lane.points.push_back(modelPoint(ticksPerQuarterNote * 4, 1.0));

    auto compiled = fixture.compile();
    REQUIRE(compiled);
    REQUIRE(compiled.automation != nullptr);

    engine::dsp::MixerStripNode* master = compiled.stripFor(fixture.project.masterMixerNode());
    REQUIRE(master != nullptr);

    AutomationFixture::renderAt(compiled, 0);
    const float atStart = master->gain();

    AutomationFixture::renderAt(compiled, fixture.tempo.frameForTick(ticksPerQuarterNote * 4));
    const float atEnd = master->gain();

    CHECK(atStart == doctest::Approx(0.0).epsilon(0.01));

    // The registry maps through the fader's cubic law: full scale is gain 4.
    CHECK(atEnd == doctest::Approx(4.0).epsilon(0.01));
}

TEST_CASE("a pan lane on a channel drives the channel strip")
{
    AutomationFixture fixture;

    project::AutomationLane& lane = fixture.project.addAutomationLane(fixture.channel, "pan");
    lane.points.push_back(modelPoint(0, 0.0));   // normalised 0 -> hard left

    auto compiled = fixture.compile();
    REQUIRE(compiled);
    REQUIRE(compiled.automation != nullptr);

    AutomationFixture::renderAt(compiled, 0);

    engine::dsp::MixerStripNode* strip = compiled.channelStripFor(fixture.channel);
    REQUIRE(strip != nullptr);
    CHECK(strip->pan() == doctest::Approx(-1.0));
}

TEST_CASE("lanes naming unknown parameters or missing targets are data, not errors")
{
    AutomationFixture fixture;

    project::AutomationLane& unknown =
        fixture.project.addAutomationLane(fixture.project.masterMixerNode(), "no.such.parameter");
    unknown.points.push_back(modelPoint(0, 0.5));

    project::AutomationLane& orphan =
        fixture.project.addAutomationLane(project::EntityId{999999}, "volume");
    orphan.points.push_back(modelPoint(0, 0.5));

    auto compiled = fixture.compile();
    REQUIRE(compiled);

    // Neither lane compiled; neither broke the graph.
    CHECK(compiled.automation == nullptr);
}

TEST_CASE("the automation node allocates nothing while rendering")
{
    AutomationFixture fixture;

    project::AutomationLane& lane =
        fixture.project.addAutomationLane(fixture.project.masterMixerNode(), "volume");
    for (int index = 0; index < 32; ++index)
        lane.points.push_back(modelPoint(static_cast<Tick>(index) * 120, (index % 2) ? 1.0 : 0.0));

    auto compiled = fixture.compile();
    REQUIRE(compiled);
    REQUIRE(compiled.automation != nullptr);

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 64);

    engine::rt::resetViolations();

    {
        const engine::rt::ScopedRealtimeContext scope;

        for (int block = 0; block < 64; ++block) {
            pool.buffer(0).clear();
            compiled.graph->process(pool.buffer(0), 64,
                                    static_cast<engine::FramePosition>(block) * 64);
        }
    }

    CHECK(engine::rt::allocationViolations() == 0);
}

// ── Commands ──────────────────────────────────────────────────────────────────

TEST_CASE("automation commands round trip")
{
    AutomationFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const project::Project original = fixture.project;

    auto add = std::make_unique<app::AddAutomationLaneCommand>(
        fixture.project.masterMixerNode(), "volume");
    app::AddAutomationLaneCommand* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));

    const project::EntityId lane = raw->laneId();
    CHECK(lane.isValid());

    REQUIRE(registry.execute(std::make_unique<app::SetAutomationPointsCommand>(
        lane,
        std::vector<project::AutomationPoint>{modelPoint(0, 0.2),
                                              modelPoint(960, 0.8)})));

    REQUIRE(fixture.project.automation().size() == 1);
    CHECK(fixture.project.automation()[0].points.size() == 2);

    // Points arrive sorted whatever order they were handed in.
    REQUIRE(registry.execute(std::make_unique<app::SetAutomationPointsCommand>(
        lane,
        std::vector<project::AutomationPoint>{modelPoint(960, 0.9), modelPoint(0, 0.1),
                                              modelPoint(480, 0.5)})));
    CHECK(fixture.project.automation()[0].points[0].tick == 0);
    CHECK(fixture.project.automation()[0].points[2].tick == 960);

    REQUIRE(registry.execute(std::make_unique<app::RemoveAutomationLaneCommand>(lane)));
    CHECK(fixture.project.automation().empty());

    while (registry.canUndo())
        REQUIRE(registry.undo());

    CHECK(fixture.project == original);

    REQUIRE(registry.redo());
    REQUIRE(fixture.project.automation().size() == 1);
    CHECK(fixture.project.automation()[0].id == lane);
}

TEST_CASE("dragging an automation point is one undo")
{
    AutomationFixture fixture;
    app::CommandRegistry registry{fixture.project};

    auto add = std::make_unique<app::AddAutomationLaneCommand>(
        fixture.project.masterMixerNode(), "volume");
    app::AddAutomationLaneCommand* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));

    for (const double value : {0.3, 0.4, 0.5, 0.6})
        REQUIRE(registry.executeMerging(std::make_unique<app::SetAutomationPointsCommand>(
            raw->laneId(), std::vector<project::AutomationPoint>{modelPoint(0, value)},
            "Move Point")));

    CHECK(registry.undoDepth() == 2);   // the lane, then the whole drag
    CHECK(fixture.project.automation()[0].points[0].value == doctest::Approx(0.6));

    REQUIRE(registry.undo());
    CHECK(fixture.project.automation()[0].points.empty());
}
