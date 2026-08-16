// FL2026 P9 (part 4) — dropping a sample into the project.
//
// A drop is a gesture, not a second way of editing: it must go through
// commands, land in one undo, and keep its ids across redo. The ids are the
// part that bites — a fresh asset id per redo would orphan every zone and clip
// written above it in the stack.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ImportCommands.h"
#include "app/commands/SamplerCommands.h"
#include "engine/audio/WavFile.h"
#include "project/Model.h"

#include <filesystem>
#include <fstream>
#include <memory>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

struct ScratchDir {
    fs::path path;

    explicit ScratchDir(const char* name)
        : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDir()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }
};

/// A real WAV on disk: the commands probe headers, so a fixture that is not a
/// file would test nothing they do.
fs::path writeWav(const fs::path& path, engine::FrameCount frames, std::size_t channels = 1,
                  engine::SampleRate rate = 48000.0)
{
    engine::AudioFileData data;
    data.sampleRate   = rate;
    data.channelCount = channels;
    data.frameCount   = frames;
    data.channels.assign(channels, std::vector<engine::Sample>(static_cast<std::size_t>(frames), 0.25F));

    REQUIRE(engine::WavFile::write(path, data));
    return path;
}

} // namespace

TEST_CASE("a sample dropped on empty rack space becomes a sampler channel")
{
    ScratchDir scratch("incdaw-drop-channel");
    const fs::path file = writeWav(scratch.path / "kick.wav", 480);

    project::Project     project;
    app::CommandRegistry registry(project);

    const std::size_t channelsBefore = project.channels().size();

    auto  command  = std::make_unique<app::ImportSampleAsChannelCommand>(file.string());
    auto* imported = command.get();

    REQUIRE(registry.execute(std::move(command)));

    REQUIRE(project.channels().size() == channelsBefore + 1);
    REQUIRE(project.audioAssets().size() == 1);

    const project::Channel& channel = project.channels().back();
    CHECK(channel.name == "kick");
    CHECK(channel.instrument == plugins::builtinSampler());
    REQUIRE(channel.samplerZones.size() == 1);
    CHECK(channel.samplerZones.front().asset == project.audioAssets().front().id);

    const project::EntityId channelId = imported->channelId();
    const project::EntityId assetId   = imported->assetId();

    REQUIRE(registry.undo());
    CHECK(project.channels().size() == channelsBefore);
    CHECK(project.audioAssets().empty());

    REQUIRE(registry.redo());
    REQUIRE(project.channels().size() == channelsBefore + 1);
    CHECK(project.channels().back().id == channelId);
    REQUIRE(project.audioAssets().size() == 1);
    CHECK(project.audioAssets().front().id == assetId);
    CHECK(project.channels().back().samplerZones.front().asset == assetId);
}

TEST_CASE("a file already in the project is shared, not imported twice")
{
    ScratchDir scratch("incdaw-drop-share");
    const fs::path file = writeWav(scratch.path / "loop.wav", 960);

    project::Project     project;
    app::CommandRegistry registry(project);

    REQUIRE(registry.execute(std::make_unique<app::ImportSampleAsChannelCommand>(file.string())));
    REQUIRE(registry.execute(std::make_unique<app::ImportSampleAsChannelCommand>(file.string())));

    CHECK(project.channels().size() == 2);
    CHECK(project.audioAssets().size() == 1);

    // Undoing the second drop keeps the asset the first one is still using.
    REQUIRE(registry.undo());
    CHECK(project.channels().size() == 1);
    CHECK(project.audioAssets().size() == 1);
}

TEST_CASE("a sample dropped on a lane becomes an audio clip at the drop tick")
{
    ScratchDir scratch("incdaw-drop-clip");
    const fs::path file = writeWav(scratch.path / "break.wav", 24000, 2);

    project::Project     project;
    app::CommandRegistry registry(project);

    const project::EntityId track = project.addTrack(project::TrackType::audio, "Audio 1").id;
    const project::Tick     drop  = engine::ticksPerQuarterNote * 4;

    auto  command  = std::make_unique<app::ImportAudioClipCommand>(track, file.string(), drop);
    auto* imported = command.get();

    REQUIRE(registry.execute(std::move(command)));
    REQUIRE(project.clips().size() == 1);

    const project::Clip& clip = project.clips().front();
    CHECK(clip.type == project::ClipType::audio);
    CHECK(clip.track == track);
    CHECK(clip.source == imported->assetId());
    CHECK(clip.name == "break");

    // Audio clips live in frames (D-013): the drop tick is converted here, once.
    CHECK(clip.start == project.tempoMap().frameForTick(drop));
    CHECK(clip.length == 24000);
    CHECK(clip.sourceOffset == 0);

    const project::EntityId clipId = clip.id;

    REQUIRE(registry.undo());
    CHECK(project.clips().empty());
    CHECK(project.audioAssets().empty());

    REQUIRE(registry.redo());
    REQUIRE(project.clips().size() == 1);
    CHECK(project.clips().front().id == clipId);
    CHECK(project.clips().front().source == imported->assetId());
}

TEST_CASE("a drop that cannot work changes nothing")
{
    ScratchDir scratch("incdaw-drop-refused");

    project::Project     project;
    app::CommandRegistry registry(project);

    const project::EntityId track = project.addTrack(project::TrackType::audio, "Audio 1").id;

    // Not a WAV at all.
    const fs::path text = scratch.path / "notes.txt";
    { std::ofstream stream(text); stream << "not audio"; }

    CHECK_FALSE(registry.execute(std::make_unique<app::ImportSampleAsChannelCommand>(text.string())));
    CHECK_FALSE(registry.execute(std::make_unique<app::ImportAudioClipCommand>(track, text.string(), 0)));

    // A track that is not there.
    const fs::path file = writeWav(scratch.path / "ok.wav", 128);
    CHECK_FALSE(registry.execute(
        std::make_unique<app::ImportAudioClipCommand>(project::EntityId{}, file.string(), 0)));

    CHECK(project.clips().empty());
    CHECK(project.audioAssets().empty());
    CHECK(project.channels().empty());
}
