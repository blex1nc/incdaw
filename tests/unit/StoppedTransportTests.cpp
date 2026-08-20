// A stopped transport must be silent.
//
// The transport keeps handing the graph one segment while stopped, on purpose:
// a synth's release, a delay's tail and input monitoring all have to keep
// coming out of the speakers after the playhead parks (Transport::processBlock).
// But that segment carries the SAME playPosition on every block — so any node
// that READS the timeline and does not know the transport stopped re-emits the
// window under the playhead forever: a note retriggered every block, or a few
// hundred frames of a clip looped at the block rate. Both are heard as a hum
// that starts when the application opens and never stops.
//
// These tests pin the contract from both sides: nothing timeline-driven sounds
// while stopped, and everything live still does.

#include "doctest.h"

#include "engine/audio/WavFile.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/midi/MidiMessage.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::FrameCount;
using incdaw::engine::Sample;

namespace fs = std::filesystem;

namespace {

constexpr FrameCount blockSize = 256;

/// A temporary directory that removes itself.
struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-stopped-" + name + "-" + std::to_string(nextSerial())))
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

/// One channel playing a note that starts at tick 0, so the parked playhead
/// sits exactly on an onset — the worst case, and the one the default project
/// lands in.
project::Project projectWithNoteAtZero()
{
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId channel = project.addChannel("Lead").id;

    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = 0;
    note.duration = engine::ticksPerQuarterNote * 2;
    note.key      = 60;
    note.value    = 100;

    auto& pattern = project.addPattern("Pattern");
    pattern.contentFor(channel).events.push_back(note);

    return project;
}

/// Renders `blocks` blocks and reports the loudest sample across all of them.
/// `playing` false renders them all at the same position, which is exactly
/// what the engine does while the transport is stopped.
Sample peakOver(const project::CompiledProjectGraph& compiled, int blocks, bool playing,
                const engine::MidiBuffer* firstBlockMidi = nullptr)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    Sample                peak     = 0.0f;
    engine::FramePosition position = 0;

    for (int index = 0; index < blocks; ++index) {
        const engine::MidiBuffer* midi = index == 0 ? firstBlockMidi : nullptr;

        compiled.graph->process(pool.buffer(0), blockSize, position, midi, playing);
        peak = std::max(peak, pool.buffer(0).peak());

        if (playing)
            position += blockSize;
    }

    return peak;
}

} // namespace

TEST_CASE("a stopped transport renders silence, however long it is left alone")
{
    project::Project project = projectWithNoteAtZero();

    engine::TempoMap map;
    map.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.maxBlockSize = blockSize;

    const auto compiled = project::compileProjectGraph(project, map, options);
    REQUIRE(compiled);

    // The fixture is not trivially silent: the same graph, with the timeline
    // advancing, plays the note. Without this the test below would pass on a
    // graph that could not make a sound at all.
    CHECK(peakOver(compiled, 8, /*playing=*/true) > 0.0f);

    const auto stopped = project::compileProjectGraph(project, map, options);
    REQUIRE(stopped);

    // Sixteen blocks is a third of a second of a hum, which is more than
    // enough to hear. Before the transport state reached the nodes, this was
    // the note retriggered sixteen times.
    CHECK(peakOver(stopped, 16, /*playing=*/false) == 0.0f);
}

TEST_CASE("a note played live while the transport is stopped keeps sounding")
{
    // The other half of the contract. The instrument node ends the SEQUENCE's
    // notes once when the transport stops; if it went on doing that every
    // block it would also cut down every note the user plays on a keyboard
    // while the project is parked — which is most of how an instrument is
    // auditioned.
    // No sequenced material at all, so the only thing that can make a sound
    // here is the note played into it.
    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId channel = project.addChannel("Lead").id;
    auto&                   pattern = project.addPattern("Pattern");
    (void)pattern.contentFor(channel);

    engine::TempoMap map;
    map.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.maxBlockSize = blockSize;

    const auto compiled = project::compileProjectGraph(project, map, options);
    REQUIRE(compiled);

    engine::MidiBuffer live;
    REQUIRE(live.insert(engine::MidiMessage::noteOn(0, 64, 100, 0)));

    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    // Block 0 carries the note on; the blocks after it carry nothing, and the
    // voice must still be there.
    compiled.graph->process(pool.buffer(0), blockSize, 0, &live, /*playing=*/false);
    CHECK(pool.buffer(0).peak() > 0.0f);

    Sample sustained = 0.0f;
    for (int index = 0; index < 4; ++index) {
        compiled.graph->process(pool.buffer(0), blockSize, 0, nullptr, /*playing=*/false);
        sustained = std::max(sustained, pool.buffer(0).peak());
    }

    CHECK(sustained > 0.0f);
}

TEST_CASE("an audio clip under a parked playhead is silent")
{
    ScratchDirectory scratch{"clip"};

    engine::AudioFileData data;
    data.sampleRate   = 48000.0;
    data.channelCount = 1;
    data.frameCount   = 48000;
    data.channels.assign(1, std::vector<Sample>(48000, 0.5f));

    const fs::path wav = scratch.path / "tone.wav";
    REQUIRE(engine::WavFile::write(wav, data));

    project::Project project;
    project.tempoMap().setSampleRate(48000.0);

    const project::EntityId track =
        project.addTrack(project::TrackType::audio, "Audio").id;

    auto& asset        = project.addAudioAsset(wav.string());
    asset.absolutePath = wav.string();
    asset.frameCount   = 48000;

    auto& clip  = project.addClip(project::ClipType::audio, track, asset.id);
    clip.start  = 0;
    clip.length = 48000;

    engine::TempoMap map;
    map.setSampleRate(48000.0);

    project::GraphCompileOptions options;
    options.maxBlockSize = blockSize;
    options.source       = project::PlaybackSource::arrangement;

    const auto compiled = project::compileProjectGraph(project, map, options);
    REQUIRE(compiled);

    CHECK(peakOver(compiled, 4, /*playing=*/true) > 0.0f);

    const auto stopped = project::compileProjectGraph(project, map, options);
    REQUIRE(stopped);
    CHECK(peakOver(stopped, 16, /*playing=*/false) == 0.0f);
}
