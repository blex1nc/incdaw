// Phase 11b — automation clips, pattern automation, and write-mode recording.
//
// The load-bearing property is the window: a placed lane writes its
// parameter only inside its clip, holds whatever it last wrote after the
// clip ends, and touches nothing before it begins. And a recorded pass must
// land undoably — over an existing lane without erasing what lies outside
// the written range.

#include "doctest.h"

#include "app/AutomationWriteSession.h"
#include "app/CommandRegistry.h"
#include "app/commands/AutomationCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <memory>
#include <vector>

using namespace incdaw;
using engine::FramePosition;
using engine::Tick;
using engine::ticksPerQuarterNote;

namespace {

/// Compiles the project with a registry whose test parameter records every
/// value the automation node writes, then renders one block per requested
/// position. 48 kHz, 120 bpm: one quarter note is 24,000 frames.
struct AutomationProbe {
    std::vector<float> written;
    int                calls = 0;

    project::ParameterRegistry registry;

    AutomationProbe()
    {
        registry = project::ParameterRegistry::withBuiltins();
        registry.registerParameter("probe", [this](engine::dsp::MixerStripNode&, float value) {
            written.push_back(value);
            ++calls;
        });
    }

    void renderAt(const project::Project& projectModel, std::vector<FramePosition> positions)
    {
        project::GraphCompileOptions options;
        options.source     = project::PlaybackSource::arrangement;
        options.parameters = &registry;

        auto compiled = project::compileProjectGraph(projectModel, projectModel.tempoMap(),
                                                     options);
        REQUIRE(bool(compiled));

        engine::AudioBufferPool pool;
        pool.allocate(1, 2, 64);

        for (const FramePosition position : positions) {
            pool.buffer(0).clear();
            compiled.graph->process(pool.buffer(0), 64, position);
        }
    }
};

project::Project makeProject()
{
    project::Project projectModel;
    projectModel.tempoMap().setSampleRate(48000.0);
    return projectModel;
}

/// A ramp 0 -> 1 across one quarter note, in lane-local ticks from zero.
project::AutomationLane& addRampLane(project::Project& projectModel)
{
    auto& lane = projectModel.addAutomationLane(projectModel.masterMixerNode(), "probe");

    project::AutomationPoint start;
    start.tick  = 0;
    start.value = 0.0;

    project::AutomationPoint end;
    end.tick  = ticksPerQuarterNote;
    end.value = 1.0;

    lane.points = {start, end};
    return lane;
}

} // namespace

TEST_CASE("a placed lane writes only inside its clip and holds after it")
{
    auto projectModel = makeProject();
    auto& lane  = addRampLane(projectModel);
    auto& track = projectModel.addTrack(project::TrackType::automation, "Auto");

    // The clip places the lane's local tick 0 at one quarter note.
    auto& clip = projectModel.addClip(project::ClipType::automation, track.id, lane.id);
    clip.startTick         = ticksPerQuarterNote;
    clip.lengthTicks       = ticksPerQuarterNote;
    clip.sourceOffsetTicks = 0;

    AutomationProbe probe;

    // Before the clip; at its start; halfway; just past its end.
    probe.renderAt(projectModel, {0, 24000, 36000, 50000});

    REQUIRE(probe.calls == 2);            // only the two blocks inside the window
    CHECK(probe.written[0] == doctest::Approx(0.0).epsilon(0.01));
    CHECK(probe.written[1] == doctest::Approx(0.5).epsilon(0.01));
}

TEST_CASE("a lane on a muted clip is silent everywhere, not global")
{
    auto projectModel = makeProject();
    auto& lane  = addRampLane(projectModel);
    auto& track = projectModel.addTrack(project::TrackType::automation, "Auto");

    auto& clip = projectModel.addClip(project::ClipType::automation, track.id, lane.id);
    clip.startTick   = 0;
    clip.lengthTicks = ticksPerQuarterNote;
    clip.muted       = true;

    AutomationProbe probe;
    probe.renderAt(projectModel, {0, 12000});

    CHECK(probe.calls == 0);
}

TEST_CASE("an unplaced lane still plays globally — the 11a behaviour")
{
    auto projectModel = makeProject();
    addRampLane(projectModel);

    AutomationProbe probe;
    probe.renderAt(projectModel, {12000});   // tick 480, mid-ramp

    REQUIRE(probe.calls == 1);
    CHECK(probe.written[0] == doctest::Approx(0.5).epsilon(0.01));
}

TEST_CASE("a pattern clip carries its pattern's automation lanes, windowed")
{
    auto projectModel = makeProject();
    auto& lane = addRampLane(projectModel);

    auto& pattern = projectModel.addPattern("P");
    pattern.length = ticksPerQuarterNote;
    pattern.automationLanes.push_back(lane.id);

    auto& track = projectModel.addTrack(project::TrackType::instrument, "T");
    auto& clip  = projectModel.addClip(project::ClipType::pattern, track.id, pattern.id);
    clip.startTick   = 2 * ticksPerQuarterNote;
    clip.lengthTicks = ticksPerQuarterNote;

    AutomationProbe probe;

    // Before the pattern clip; halfway into it.
    probe.renderAt(projectModel, {24000, 60000});

    REQUIRE(probe.calls == 1);
    CHECK(probe.written[0] == doctest::Approx(0.5).epsilon(0.01));
}

TEST_CASE("a recorded pass lands as lane, clip and track — and undoes as one")
{
    auto projectModel = makeProject();
    app::CommandRegistry registry{projectModel};

    app::AutomationWriteSession session;
    session.setEnabled(true);

    // A fader ride: many raw points, mostly redundant.
    for (Tick tick = 0; tick <= ticksPerQuarterNote; tick += 10)
        session.capture(projectModel.masterMixerNode(), "volume", tick,
                        static_cast<double>(tick) / ticksPerQuarterNote);

    auto commands = session.finish();
    REQUIRE(commands.size() == 1);
    REQUIRE(registry.execute(std::move(commands.front())));

    REQUIRE(projectModel.automation().size() == 1);
    REQUIRE(projectModel.tracks().size() == 1);
    CHECK(projectModel.tracks().front().type == project::TrackType::automation);
    REQUIRE(projectModel.clips().size() == 1);
    CHECK(projectModel.clips().front().type == project::ClipType::automation);
    CHECK(projectModel.clips().front().source == projectModel.automation().front().id);

    // Thinned: a straight ramp bends nowhere, so two points carry it.
    const std::size_t pointCount = projectModel.automation().front().points.size();
    CHECK(pointCount >= 2);
    CHECK(pointCount <= 4);

    const auto laneId = projectModel.automation().front().id;

    REQUIRE(registry.undo());
    CHECK(projectModel.automation().empty());
    CHECK(projectModel.clips().empty());
    CHECK(projectModel.tracks().empty());

    REQUIRE(registry.redo());
    CHECK(projectModel.automation().front().id == laneId);
}

TEST_CASE("writing over an existing lane replaces the range and keeps the rest")
{
    auto projectModel = makeProject();
    app::CommandRegistry registry{projectModel};

    auto& lane = projectModel.addAutomationLane(projectModel.masterMixerNode(), "volume");

    project::AutomationPoint before, inside, after;
    before.tick = 0;                        before.value = 0.1;
    inside.tick = ticksPerQuarterNote;      inside.value = 0.2;
    after.tick  = 4 * ticksPerQuarterNote;  after.value  = 0.3;
    lane.points = {before, inside, after};

    const auto previous = lane.points;

    // The pass covers ticks [960, 1920]: `inside` must vanish, the ends stay.
    app::AutomationWriteSession session;
    session.setEnabled(true);
    session.capture(projectModel.masterMixerNode(), "volume", ticksPerQuarterNote, 0.9);
    session.capture(projectModel.masterMixerNode(), "volume", 2 * ticksPerQuarterNote, 0.8);

    auto commands = session.finish();
    REQUIRE(commands.size() == 1);
    REQUIRE(registry.execute(std::move(commands.front())));

    REQUIRE(projectModel.automation().size() == 1);
    const auto& points = projectModel.automation().front().points;

    REQUIRE(points.size() == 4);
    CHECK(points[0].value == 0.1);   // kept: before the range
    CHECK(points[1].value == 0.9);
    CHECK(points[2].value == 0.8);
    CHECK(points[3].value == 0.3);   // kept: after the range

    CHECK(projectModel.clips().empty());   // an existing lane gets no new clip

    REQUIRE(registry.undo());
    CHECK(projectModel.automation().front().points == previous);
}

TEST_CASE("the session restarts a stream when the transport wraps")
{
    auto projectModel = makeProject();

    app::AutomationWriteSession session;
    session.setEnabled(true);

    session.capture(projectModel.masterMixerNode(), "volume", 1000, 0.1);
    session.capture(projectModel.masterMixerNode(), "volume", 2000, 0.2);
    session.capture(projectModel.masterMixerNode(), "volume", 500, 0.5);   // loop wrap

    auto commands = session.finish();
    REQUIRE(commands.size() == 1);

    app::CommandRegistry registry{projectModel};
    REQUIRE(registry.execute(std::move(commands.front())));

    // Only the post-wrap capture survived.
    REQUIRE(projectModel.automation().size() == 1);
    REQUIRE(projectModel.automation().front().points.size() == 1);
    CHECK(projectModel.automation().front().points.front().tick == 500);
}
