// Phase 19 — QA: the stress half.
//
// Large-project stress (a project an order of magnitude beyond a busy
// session survives save, load, compile and render) and long-session
// stability (hundreds of edit-and-rebuild cycles with audio processed
// between them, the exact loop a day of work performs). The assertions are
// correctness, not speed — Phase 18 owns the numbers.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/OfflineRender.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <cmath>
#include <filesystem>
#include <string>

using namespace incdaw;
using incdaw::engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-stress-" + name + "-" + std::to_string(nextSerial())))
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDirectory()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }

    fs::path path;

private:
    static int nextSerial()
    {
        static int serial = 0;
        return ++serial;
    }
};

/// Big by session standards: 96 channels, 24 patterns of 512 notes each,
/// 192 clips, automation on every fourth channel, a full master chain.
project::Project makeLargeProject()
{
    project::Project large;

    large.metadata().title = "Stress";

    std::vector<project::EntityId> channels;
    for (int index = 0; index < 96; ++index) {
        auto& channel = large.addChannel("Ch " + std::to_string(index));
        channels.push_back(channel.id);
    }

    const auto track = large.addTrack(project::TrackType::instrument, "T").id;

    std::vector<project::EntityId> patterns;
    for (int patternIndex = 0; patternIndex < 24; ++patternIndex) {
        auto& pattern  = large.addPattern("P" + std::to_string(patternIndex));
        pattern.length = ticksPerQuarterNote * 16;

        for (int noteIndex = 0; noteIndex < 512; ++noteIndex) {
            project::MidiEvent note;
            note.type     = project::MidiEventType::note;
            note.tick     = (noteIndex * 37) % (ticksPerQuarterNote * 16);
            note.duration = 120 + (noteIndex % 5) * 120;
            note.key      = 24 + (noteIndex * 7) % 84;
            note.value    = 40 + (noteIndex * 13) % 88;

            pattern.contentFor(channels[static_cast<std::size_t>(
                                   (patternIndex * 512 + noteIndex) % 96)])
                .events.push_back(note);
        }

        patterns.push_back(pattern.id);
    }

    for (int clipIndex = 0; clipIndex < 192; ++clipIndex) {
        auto& clip = large.addClip(project::ClipType::pattern, track,
                                   patterns[static_cast<std::size_t>(clipIndex % 24)]);
        clip.startTick   = clipIndex * ticksPerQuarterNote * 4;
        clip.lengthTicks = ticksPerQuarterNote * 16;
    }

    for (int index = 0; index < 96; index += 4) {
        auto& lane = large.addAutomationLane(channels[static_cast<std::size_t>(index)],
                                             "volume");
        lane.points.push_back({0, 0.2, project::AutomationCurve::linear, 0.0});
        lane.points.push_back({ticksPerQuarterNote * 64, 1.0,
                               project::AutomationCurve::smooth, 0.0});
    }

    project::MixerNode* master = large.findMixerNode(large.masterMixerNode());
    for (const char* uid : {"incdaw.eq", "incdaw.compressor", "incdaw.limiter"}) {
        project::PluginSlot slot;
        slot.id     = large.ids().next();
        slot.plugin = {plugins::Format::builtin, uid};
        master->inserts.push_back(slot);
    }

    return large;
}

} // namespace

TEST_CASE("STRESS: a large project survives save, load, compile and render intact")
{
    ScratchDirectory scratch{"large"};

    const project::Project original = makeLargeProject();

    // Save and load: identity.
    const fs::path packagePath = scratch.path / "Stress.incdaw";
    REQUIRE(project::ProjectFile::save(original, packagePath));

    project::Project loaded;
    REQUIRE(project::ProjectFile::load(loaded, packagePath));
    CHECK(loaded == original);

    // Compile: every channel with notes lands in the graph.
    const engine::TempoMap map{140.0, 48000.0};

    project::GraphCompileOptions options;
    options.source = project::PlaybackSource::arrangement;

    const auto compiled = project::compileProjectGraph(loaded, map, options);
    REQUIRE(compiled);
    CHECK(compiled.channels.size() == 96);

    // Render a slice of the middle: finite, non-silent audio.
    project::RenderOptions render;
    render.regionStart  = 48000 * 10;
    render.regionLength = 48000;
    render.tailSeconds  = 0.0;

    const auto rendered = project::renderProject(loaded, map, render);
    REQUIRE(rendered);

    engine::Sample peak = 0.0f;
    for (const auto& channel : rendered.audio.channels)
        for (const engine::Sample sample : channel) {
            REQUIRE(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }

    CHECK(peak > 0.001f);
}

TEST_CASE("STRESS: hundreds of edit-rebuild-process cycles stay correct")
{
    // The long-session loop: edit the model, rebuild the graph, process
    // audio, repeat — with undo mixed in, because a real session undoes.
    project::Project session;

    const auto channel = session.addChannel("Keys").id;
    auto&      pattern = session.addPattern("P");
    pattern.length     = ticksPerQuarterNote * 4;

    const engine::TempoMap map{120.0, 48000.0};

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 256);

    project::GraphCompileOptions options;
    options.maxBlockSize = 256;

    for (int cycle = 0; cycle < 400; ++cycle) {
        // The edit: add a note; every eighth cycle, remove one instead.
        auto* content = &session.patterns()[0].contentFor(channel);

        if (cycle % 8 == 7 && !content->events.empty()) {
            content->events.pop_back();
        } else {
            project::MidiEvent note;
            note.type     = project::MidiEventType::note;
            note.tick     = (cycle * 53) % (ticksPerQuarterNote * 4);
            note.duration = 240;
            note.key      = 36 + cycle % 48;
            note.value    = 100;
            content->events.push_back(note);
        }

        // The rebuild-and-swap every edit performs.
        const auto compiled = project::compileProjectGraph(session, map, options);
        REQUIRE(compiled);

        // A block of audio through the new graph, at a moving position.
        const auto view = pool.buffer(0);
        compiled.graph->process(view, 256, static_cast<engine::FramePosition>(cycle) * 256);

        for (engine::FrameCount frame = 0; frame < 256; ++frame)
            REQUIRE(std::isfinite(view.channel(0)[frame]));
    }

    // 400 cycles: 350 adds and 50 removes leave 300 notes standing.
    CHECK(session.patterns()[0].contentFor(channel).events.size() == 300);
}
