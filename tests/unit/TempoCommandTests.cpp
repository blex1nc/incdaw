// Tempo, time signature and the metronome — the control bar's last read-only
// numbers, made editable.
//
// The engine has carried all three since Phase 3: the tempo map holds tempo AND
// signature events, the file format serializes both, and MetronomeNode has
// existed (and been the instrument the transport is measured with) since the
// transport was built. What was missing was the path from a click to the model
// and back out through the graph — which is what these tests pin.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/TempoCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <algorithm>
#include <memory>

using namespace incdaw;

namespace {

constexpr engine::FrameCount blockSize = 512;

/// Renders `blocks` blocks from frame 0 with the timeline advancing, and
/// reports the loudest sample seen.
engine::Sample peakOf(const project::CompiledProjectGraph& compiled, int blocks)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    engine::Sample        peak     = 0.0f;
    engine::FramePosition position = 0;

    for (int index = 0; index < blocks; ++index) {
        compiled.graph->process(pool.buffer(0), blockSize, position, nullptr, true);
        peak     = std::max(peak, pool.buffer(0).peak());
        position += blockSize;
    }

    return peak;
}

} // namespace

TEST_CASE("setting the tempo is undoable, and a drag is one entry")
{
    project::Project  project;
    app::CommandRegistry registry{project};

    REQUIRE(project.tempoMap().tempoAtTick(0) == doctest::Approx(120.0));

    CHECK(registry.execute(std::make_unique<app::SetTempoCommand>(140.0)));
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(140.0));

    (void)registry.undo();
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(120.0));

    (void)registry.redo();
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(140.0));

    SUBCASE("the steps of a drag fold into the entry the drag opened")
    {
        const std::size_t before = registry.undoDepth();

        for (double tempo : {141.0, 142.5, 143.75, 145.0})
            CHECK(registry.executeMerging(std::make_unique<app::SetTempoCommand>(tempo)));

        CHECK(registry.undoDepth() == before);   // merged, not stacked
        CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(145.0));

        // One undo returns to before the gesture, not to its second-to-last
        // position.
        (void)registry.undo();
        CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(120.0));
    }

    SUBCASE("a tempo the map could not use is refused at the edge, not passed on")
    {
        CHECK(app::SetTempoCommand::clampTempo(0.0) == doctest::Approx(20.0));
        CHECK(app::SetTempoCommand::clampTempo(-5.0) == doctest::Approx(20.0));
        CHECK(app::SetTempoCommand::clampTempo(1.0e9) == doctest::Approx(999.0));
    }

    SUBCASE("setting the tempo it already has is not an undo entry")
    {
        CHECK_FALSE(registry.execute(std::make_unique<app::SetTempoCommand>(140.0)));
    }
}

TEST_CASE("a tempo edit leaves later tempo changes alone")
{
    project::Project project;
    project.tempoMap().setTempoEvents({{0, 120.0}, {engine::ticksPerQuarterNote * 16, 90.0}});

    app::CommandRegistry registry{project};
    CHECK(registry.execute(std::make_unique<app::SetTempoCommand>(150.0)));

    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(150.0));
    CHECK(project.tempoMap().tempoAtTick(engine::ticksPerQuarterNote * 16)
          == doctest::Approx(90.0));
}

TEST_CASE("the time signature is project state, and undoable")
{
    project::Project     project;
    app::CommandRegistry registry{project};

    CHECK(project.tempoMap().timeSignatureAtTick(0).numerator == 4);

    CHECK(registry.execute(std::make_unique<app::SetTimeSignatureCommand>(7, 8)));

    const engine::TimeSignature seven = project.tempoMap().timeSignatureAtTick(0);
    CHECK(seven.numerator == 7);
    CHECK(seven.denominator == 8);
    CHECK(seven.ticksPerBar() == engine::ticksPerQuarterNote * 4 / 8 * 7);

    (void)registry.undo();
    CHECK(project.tempoMap().timeSignatureAtTick(0).numerator == 4);

    SUBCASE("a denominator that is not a note value is refused")
    {
        CHECK_FALSE(registry.execute(std::make_unique<app::SetTimeSignatureCommand>(4, 5)));
        CHECK_FALSE(registry.execute(std::make_unique<app::SetTimeSignatureCommand>(0, 4)));
    }
}

TEST_CASE("the metronome is in the graph exactly when it is switched on")
{
    // An empty project: anything audible can only be the click.
    project::Project project;
    project.addChannel("Empty");

    engine::TempoMap map;
    map.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.maxBlockSize = blockSize;

    const auto without = project::compileProjectGraph(project, map, options);
    REQUIRE(without);
    CHECK(peakOf(without, 8) == 0.0f);

    options.metronomeEnabled = true;

    const auto with = project::compileProjectGraph(project, map, options);
    REQUIRE(with);
    CHECK(with.graph->nodeCount() > without.graph->nodeCount());
    CHECK(peakOf(with, 8) > 0.0f);
}

TEST_CASE("a compiled graph renders against its own tempo map")
{
    // The reason a tempo edit is safe while audio runs: the nodes point at the
    // map the graph owns, not at the caller's. Rewriting the caller's map must
    // not reach a graph that is already rendering.
    project::Project project;
    project.addChannel("Lead");

    engine::TempoMap map{120.0, 48000.0};

    project::GraphCompileOptions options;
    options.maxBlockSize = blockSize;

    const auto compiled = project::compileProjectGraph(project, map, options);
    REQUIRE(compiled);
    REQUIRE(compiled.tempoMap != nullptr);

    CHECK(compiled.tempoMap.get() != &map);
    CHECK(compiled.tempoMap->tempoAtTick(0) == doctest::Approx(120.0));

    map.setTempoEvents({{0, 200.0}});

    CHECK(compiled.tempoMap->tempoAtTick(0) == doctest::Approx(120.0));
}
