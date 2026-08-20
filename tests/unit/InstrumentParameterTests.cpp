// UI build-out increment 4 — stored instrument parameters (format 1.5,
// docs/DECISIONS.md D-034).
//
// The load-bearing claims: a value stored on the channel shapes the sound
// through the compiler exactly as the same value written to the live sink
// would (the model is the source of truth at every build), the format
// round-trips and its frozen fixture loads, and the command is undoable
// with drag merging.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/SamplerCommands.h"
#include "engine/audio/WavFile.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/instrument/SimpleSynth.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <vector>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

constexpr auto gainParam =
    static_cast<std::uint32_t>(engine::SimpleSynthParam::gain);

/// One synth channel playing one long note — enough signal for a gain
/// difference to be unmistakable in the very first block.
project::Project synthProject(project::EntityId& channelOut)
{
    project::Project project;

    auto& channel      = project.addChannel("Synth");
    channel.instrument = plugins::builtinSimpleSynth();
    channelOut         = channel.id;

    auto& pattern = project.addPattern("Pattern");

    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = 0;
    note.duration = engine::ticksPerQuarterNote * 4;
    note.key      = 60;
    note.value    = 100;
    pattern.contentFor(channel.id).events.push_back(note);

    return project;
}

/// Drives `count` blocks and appends every sample to `out`.
void drive(const project::CompiledProjectGraph& compiled, std::size_t count,
           std::vector<engine::Sample>& out)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 256);

    for (std::size_t block = 0; block < count; ++block) {
        auto buffer = pool.buffer(0);
        compiled.graph->process(buffer, 256, static_cast<engine::FramePosition>(block * 256));

        for (std::size_t channel = 0; channel < buffer.channelCount(); ++channel)
            for (std::size_t frame = 0; frame < 256; ++frame)
                out.push_back(buffer.channel(channel)[frame]);
    }
}

} // namespace

TEST_CASE("a stored instrument parameter shapes the sound exactly as the live sink would")
{
    const engine::TempoMap map{120.0, 48000.0};

    project::EntityId channelA;
    project::Project  withStored = synthProject(channelA);
    withStored.findChannel(channelA)->instrumentParameters.push_back({gainParam, 1.0});

    project::EntityId channelB;
    project::Project  withSink = synthProject(channelB);

    const auto compiledStored =
        project::compileProjectGraph(withStored, map, project::GraphCompileOptions{});
    REQUIRE(compiledStored);

    const auto compiledSink =
        project::compileProjectGraph(withSink, map, project::GraphCompileOptions{});
    REQUIRE(compiledSink);

    // The same value through the live node's sink — the write a panel or a
    // MIDI mapping performs.
    engine::InstrumentNode* node = compiledSink.instrumentFor(channelB);
    REQUIRE(node != nullptr);
    REQUIRE(node->parameterSink() != nullptr);
    node->parameterSink()->setParameter(gainParam, 1.0);

    std::vector<engine::Sample> stored;
    std::vector<engine::Sample> sunk;
    drive(compiledStored, 4, stored);
    drive(compiledSink, 4, sunk);

    CHECK(stored == sunk);

    // And the value audibly did something: defaults render differently.
    project::EntityId channelC;
    project::Project  defaults = synthProject(channelC);

    const auto compiledDefaults =
        project::compileProjectGraph(defaults, map, project::GraphCompileOptions{});
    REQUIRE(compiledDefaults);

    std::vector<engine::Sample> untouched;
    drive(compiledDefaults, 4, untouched);

    CHECK(stored != untouched);
}

TEST_CASE("instrument parameters round-trip through the project file")
{
    const fs::path package =
        fs::temp_directory_path() / "incdaw-instrument-params" / "RoundTrip.incdaw";
    fs::remove_all(package.parent_path());
    fs::create_directories(package.parent_path());

    project::EntityId channelId;
    project::Project  saved = synthProject(channelId);
    saved.findChannel(channelId)->instrumentParameters.push_back({gainParam, 0.9});
    saved.findChannel(channelId)->instrumentParameters.push_back(
        {static_cast<std::uint32_t>(engine::SimpleSynthParam::attackSeconds), 0.25});

    REQUIRE(project::ProjectFile::save(saved, package));

    project::Project loaded;
    REQUIRE(project::ProjectFile::load(loaded, package));

    CHECK(loaded == saved);

    fs::remove_all(package.parent_path());
}

TEST_CASE("the v1.5 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.5" / "Fixture.incdaw";

    project::Project project;
    const auto       result = project::ProjectFile::load(project, fixture);
    REQUIRE(result);
    CHECK(result.migrated);   // 1.5 predates markers and stereo width (1.6)

    CHECK(project.metadata().title == "Format v1.5 fixture");

    REQUIRE(!project.channels().empty());
    const project::Channel& kick = project.channels().front();

    REQUIRE(kick.instrumentParameters.size() == 2);
    CHECK(kick.instrumentParameters[0].parameterId == 4);
    CHECK(kick.instrumentParameters[0].value == doctest::Approx(2200.0));
    CHECK(kick.instrumentParameters[1].parameterId == 8);
    CHECK(kick.instrumentParameters[1].value == doctest::Approx(0.35));
}

TEST_CASE("SetInstrumentParameterCommand is undoable and merges a drag")
{
    project::EntityId channelId;
    project::Project  project = synthProject(channelId);

    app::CommandRegistry registry{project};

    // A parameter never touched: undo removes the entry entirely, so "at the
    // default" and "stored at the default's value" stay distinguishable.
    REQUIRE(registry.execute(
        std::make_unique<app::SetInstrumentParameterCommand>(channelId, gainParam, 0.8)));

    auto& parameters = project.findChannel(channelId)->instrumentParameters;
    REQUIRE(parameters.size() == 1);
    CHECK(parameters[0].value == doctest::Approx(0.8));

    // The drag: consecutive edits of the same parameter are one undo entry.
    REQUIRE(registry.executeMerging(
        std::make_unique<app::SetInstrumentParameterCommand>(channelId, gainParam, 0.6)));
    CHECK(registry.undoDepth() == 1);
    CHECK(parameters[0].value == doctest::Approx(0.6));

    registry.undo();
    CHECK(project.findChannel(channelId)->instrumentParameters.empty());

    registry.redo();
    REQUIRE(project.findChannel(channelId)->instrumentParameters.size() == 1);
    CHECK(project.findChannel(channelId)->instrumentParameters[0].value
          == doctest::Approx(0.6));
}

TEST_CASE("sampler zone edit and removal are undoable")
{
    project::EntityId channelId;
    project::Project  project = synthProject(channelId);

    project::Channel* channel = project.findChannel(channelId);
    channel->samplerZones.push_back({});
    channel->samplerZones[0].rootKey = 60;

    app::CommandRegistry registry{project};

    project::ChannelSamplerZone edited = channel->samplerZones[0];
    edited.rootKey                     = 48;
    edited.keyHigh                     = 72;

    REQUIRE(registry.execute(
        std::make_unique<app::SetSamplerZoneCommand>(channelId, 0, edited)));
    CHECK(channel->samplerZones[0].rootKey == 48);

    registry.undo();
    CHECK(channel->samplerZones[0].rootKey == 60);
    registry.redo();

    REQUIRE(registry.execute(std::make_unique<app::RemoveSamplerZoneCommand>(channelId, 0)));
    CHECK(channel->samplerZones.empty());

    registry.undo();
    REQUIRE(channel->samplerZones.size() == 1);
    CHECK(channel->samplerZones[0].rootKey == 48);

    // Out of range is a refusal, not a crash.
    CHECK_FALSE(registry.execute(
        std::make_unique<app::RemoveSamplerZoneCommand>(channelId, 7)));
}

TEST_CASE("the instrument catalogue answers by uid")
{
    REQUIRE(engine::findBuiltinInstrument("incdaw.sampler") != nullptr);
    CHECK(engine::findBuiltinInstrument("incdaw.sampler")->parameterCount == 10);
    CHECK(engine::findBuiltinInstrument("incdaw.nope") == nullptr);
}

TEST_CASE("AddSamplerZoneCommand appends a layer and undoes cleanly")
{
    const fs::path scratch = fs::temp_directory_path() / "incdaw-add-zone";
    fs::remove_all(scratch);
    fs::create_directories(scratch);

    engine::AudioFileData data;
    data.sampleRate   = 48000.0;
    data.channelCount = 1;
    data.frameCount   = 480;
    data.channels.assign(1, std::vector<engine::Sample>(480, 0.25f));
    const fs::path wav = scratch / "layer.wav";
    REQUIRE(engine::WavFile::write(wav, data));

    project::EntityId channelId;
    project::Project  project = synthProject(channelId);

    app::CommandRegistry registry{project};

    // Not a sampler yet: adding a layer is refused, never a conversion —
    // that whole gesture belongs to LoadSampleCommand.
    CHECK_FALSE(registry.execute(
        std::make_unique<app::AddSamplerZoneCommand>(channelId, wav.string())));

    project.findChannel(channelId)->instrument = plugins::builtinSampler();

    REQUIRE(registry.execute(
        std::make_unique<app::AddSamplerZoneCommand>(channelId, wav.string())));
    CHECK(project.findChannel(channelId)->samplerZones.size() == 1);
    CHECK(project.audioAssets().size() == 1);

    // The same file again layers a second zone over the SAME asset.
    REQUIRE(registry.execute(
        std::make_unique<app::AddSamplerZoneCommand>(channelId, wav.string())));
    CHECK(project.findChannel(channelId)->samplerZones.size() == 2);
    CHECK(project.audioAssets().size() == 1);

    registry.undo();
    CHECK(project.findChannel(channelId)->samplerZones.size() == 1);
    CHECK(project.audioAssets().size() == 1);   // the first command minted it

    registry.undo();
    CHECK(project.findChannel(channelId)->samplerZones.empty());
    CHECK(project.audioAssets().empty());

    registry.redo();
    CHECK(project.audioAssets().size() == 1);

    fs::remove_all(scratch);
}
