// Phase 12 (part 5) — the audio editor's verbs.
//
// The load-bearing property is exact undo: every destructive edit must come
// back bit-identically under Cmd+Z, including trim, which changes the file's
// length. An editor whose undo is approximately right destroys recordings
// slowly and invisibly.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/AudioEditCommands.h"
#include "engine/audio/AudioEdits.h"
#include "engine/audio/WaveformOverview.h"
#include "engine/audio/WavFile.h"
#include "project/Model.h"

#include <cmath>
#include <filesystem>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
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

Sample tone(FrameCount frame, std::size_t channel = 0)
{
    return static_cast<Sample>(0.1 + 0.4 * std::sin(0.021 * static_cast<double>(frame)
                                                    + static_cast<double>(channel)));
}

AudioFileData makeAudio(std::size_t channels, FrameCount frames)
{
    AudioFileData data;
    data.sampleRate   = 48000.0;
    data.channelCount = channels;
    data.frameCount   = frames;
    data.channels.assign(channels, std::vector<Sample>(static_cast<std::size_t>(frames)));

    for (std::size_t channel = 0; channel < channels; ++channel)
        for (FrameCount frame = 0; frame < frames; ++frame)
            data.channels[channel][static_cast<std::size_t>(frame)] = tone(frame, channel);

    return data;
}

} // namespace

// ── The operations ───────────────────────────────────────────────────────────

TEST_CASE("gain, silence and fades touch exactly the region")
{
    auto data = makeAudio(2, 1000);

    edits::applyGain(data, {100, 200}, 0.5f);
    edits::silence(data, {300, 400});
    edits::fadeIn(data, {500, 600});
    edits::fadeOut(data, {700, 800});

    for (FrameCount frame = 0; frame < 1000; ++frame) {
        const Sample original = tone(frame);
        const Sample actual   = data.channels[0][static_cast<std::size_t>(frame)];

        if (frame >= 100 && frame < 200)
            REQUIRE(actual == original * 0.5f);
        else if (frame >= 300 && frame < 400)
            REQUIRE(actual == 0.0f);
        else if (frame >= 500 && frame < 600)
            REQUIRE(actual == original * (static_cast<Sample>(frame - 500) / 100.0f));
        else if (frame >= 700 && frame < 800)
            REQUIRE(actual == original * (static_cast<Sample>(800 - frame) / 100.0f));
        else
            REQUIRE(actual == original);   // everything outside every region is untouched
    }
}

TEST_CASE("normalize scales the region peak to the target and refuses silence")
{
    auto data = makeAudio(1, 1000);

    REQUIRE(edits::normalize(data, {0, 500}, 1.0f));
    const Sample peak = edits::peakIn(data, {0, 500});
    CHECK(std::abs(peak - 1.0f) < 1.0e-6f);

    edits::silence(data, {600, 700});
    CHECK_FALSE(edits::normalize(data, {600, 700}));
}

TEST_CASE("reverse is its own inverse and trim keeps exactly the region")
{
    auto data = makeAudio(2, 1000);
    const auto original = data;

    edits::reverse(data, {100, 900});
    CHECK(data.channels[0][100] == original.channels[0][899]);

    edits::reverse(data, {100, 900});
    for (FrameCount frame = 0; frame < 1000; ++frame)
        REQUIRE(data.channels[0][static_cast<std::size_t>(frame)]
                == original.channels[0][static_cast<std::size_t>(frame)]);

    edits::trimTo(data, {250, 750});
    REQUIRE(data.frameCount == 500);
    for (FrameCount frame = 0; frame < 500; ++frame)
        REQUIRE(data.channels[1][static_cast<std::size_t>(frame)]
                == original.channels[1][static_cast<std::size_t>(frame + 250)]);
}

TEST_CASE("regions are clamped, never trusted")
{
    auto data = makeAudio(1, 100);

    edits::applyGain(data, {-50, 20}, 2.0f);      // clamps to [0, 20)
    edits::applyGain(data, {90, 500}, 0.0f);      // clamps to [90, 100)
    edits::silence(data, {70, 30});               // inverted: empty, no-op

    CHECK(data.channels[0][0] == tone(0) * 2.0f);
    CHECK(data.channels[0][95] == 0.0f);
    CHECK(data.channels[0][50] == tone(50));
}

// ── The overview ─────────────────────────────────────────────────────────────

TEST_CASE("the overview built from the file equals the one built from memory")
{
    ScratchDir scratch{"incdaw-overview"};

    const auto data = makeAudio(2, 70000);   // spans multiple reader chunks
    REQUIRE(bool(WavFile::write(scratch.path / "wave.wav", data)));

    WaveformOverview fromFile;
    REQUIRE(bool(WaveformOverview::build(scratch.path / "wave.wav", fromFile)));

    WaveformOverview fromMemory;
    WaveformOverview::build(data, fromMemory);

    REQUIRE(fromFile.bucketCount() == fromMemory.bucketCount());
    REQUIRE(fromFile.channelCount == 2);
    REQUIRE(fromFile.frameCount == 70000);

    for (std::size_t channel = 0; channel < 2; ++channel)
        for (std::size_t bucket = 0; bucket < fromFile.bucketCount(); ++bucket) {
            REQUIRE(fromFile.channels[channel][bucket].low
                    == fromMemory.channels[channel][bucket].low);
            REQUIRE(fromFile.channels[channel][bucket].high
                    == fromMemory.channels[channel][bucket].high);
        }

    // Spot-check one bucket against the raw samples.
    Sample low = 1.0e9f, high = -1.0e9f;
    for (FrameCount frame = 256; frame < 512; ++frame) {
        low  = std::min(low, data.channels[0][static_cast<std::size_t>(frame)]);
        high = std::max(high, data.channels[0][static_cast<std::size_t>(frame)]);
    }

    CHECK(fromFile.channels[0][1].low == low);
    CHECK(fromFile.channels[0][1].high == high);
}

// ── The commands ─────────────────────────────────────────────────────────────

namespace {

struct EditFixture {
    ScratchDir        scratch{"incdaw-editcmd"};
    project::Project  project;
    project::EntityId assetId;
    fs::path          file;

    EditFixture()
    {
        const auto data = makeAudio(2, 5000);
        file = scratch.path / "take.wav";
        REQUIRE(bool(WavFile::write(file, data)));

        auto& asset = project.addAudioAsset(file.string());
        asset.sampleRate   = 48000.0;
        asset.frameCount   = 5000;
        asset.channelCount = 2;
        assetId = asset.id;
    }

    [[nodiscard]] AudioFileData load() const
    {
        AudioFileData data;
        REQUIRE(bool(WavFile::read(file, data)));
        return data;
    }
};

} // namespace

TEST_CASE("a region edit rewrites the file and undo restores it bit-exactly")
{
    EditFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto original = fixture.load();

    REQUIRE(registry.execute(std::make_unique<app::EditAssetRegionCommand>(
        fixture.assetId, edits::Region{1000, 2000}, app::AudioEditOp::gain, 0.25f)));

    auto edited = fixture.load();
    CHECK(edited.channels[0][1500] == original.channels[0][1500] * 0.25f);
    CHECK(edited.channels[0][500] == original.channels[0][500]);

    REQUIRE(registry.undo());

    const auto restored = fixture.load();
    for (std::size_t channel = 0; channel < 2; ++channel)
        for (FrameCount frame = 0; frame < 5000; ++frame)
            REQUIRE(restored.channels[channel][static_cast<std::size_t>(frame)]
                    == original.channels[channel][static_cast<std::size_t>(frame)]);

    // Redo writes the recorded result — applying it twice must not compound.
    REQUIRE(registry.redo());
    const auto redone = fixture.load();
    CHECK(redone.channels[0][1500] == original.channels[0][1500] * 0.25f);
}

TEST_CASE("trim shortens the file and undo reassembles the original")
{
    EditFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto original = fixture.load();

    REQUIRE(registry.execute(
        std::make_unique<app::TrimAssetCommand>(fixture.assetId, edits::Region{1000, 4000})));

    auto trimmed = fixture.load();
    REQUIRE(trimmed.frameCount == 3000);
    CHECK(trimmed.channels[0][0] == original.channels[0][1000]);
    CHECK(fixture.project.audioAssets().front().frameCount == 3000);

    REQUIRE(registry.undo());

    const auto restored = fixture.load();
    REQUIRE(restored.frameCount == 5000);
    CHECK(fixture.project.audioAssets().front().frameCount == 5000);

    for (std::size_t channel = 0; channel < 2; ++channel)
        for (FrameCount frame = 0; frame < 5000; ++frame)
            REQUIRE(restored.channels[channel][static_cast<std::size_t>(frame)]
                    == original.channels[channel][static_cast<std::size_t>(frame)]);

    // Redo trims again, to the same result.
    REQUIRE(registry.redo());
    CHECK(fixture.load().frameCount == 3000);
}

TEST_CASE("extract, delete and insert are exact and refuse mismatches")
{
    const auto data = makeAudio(2, 100);

    // Copy: the piece is the region, byte for byte.
    const auto piece = engine::edits::extractRegion(data, {20, 30});
    CHECK(piece.frameCount == 10);
    CHECK(piece.channels[0][0] == data.channels[0][20]);
    CHECK(piece.channels[1][9] == data.channels[1][29]);

    // Delete closes the gap.
    auto shortened = data;
    engine::edits::deleteRegion(shortened, {20, 30});
    CHECK(shortened.frameCount == 90);
    CHECK(shortened.channels[0][20] == data.channels[0][30]);

    // Insert at the cut restores the original exactly.
    auto restored = shortened;
    REQUIRE(engine::edits::insertAudio(restored, 20, piece));
    CHECK(restored.channels == data.channels);
    CHECK(restored.frameCount == data.frameCount);

    // Mismatched channel count or rate refuses and leaves data untouched.
    auto mono         = makeAudio(1, 10);
    auto beforeRefuse = restored;
    CHECK_FALSE(engine::edits::insertAudio(restored, 0, mono));
    CHECK(restored.channels == beforeRefuse.channels);

    auto wrongRate       = piece;
    wrongRate.sampleRate = 44100.0;
    CHECK_FALSE(engine::edits::insertAudio(restored, 0, wrongRate));
}

TEST_CASE("cut-paste round-trips through the commands, undo included")
{
    EditFixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto original = fixture.load();

    // Cut [1000, 2000): the shell copies first, then deletes undoably.
    const auto clipboard = engine::edits::extractRegion(original, {1000, 2000});

    REQUIRE(registry.execute(
        std::make_unique<app::DeleteAudioRegionCommand>(fixture.assetId,
                                                        engine::edits::Region{1000, 2000})));
    CHECK(fixture.load().frameCount == 4000);

    // Paste it back where it came from: the file round-trips bit-exactly.
    REQUIRE(registry.execute(
        std::make_unique<app::InsertAudioCommand>(fixture.assetId, 1000, clipboard)));

    const auto pasted = fixture.load();
    CHECK(pasted.frameCount == original.frameCount);
    CHECK(pasted.channels == original.channels);

    // Undo peels the paste, then the delete — back to the original twice.
    registry.undo();
    CHECK(fixture.load().frameCount == 4000);

    registry.undo();
    const auto restored = fixture.load();
    CHECK(restored.channels == original.channels);

    // Redo replays the recorded result.
    registry.redo();
    registry.redo();
    CHECK(fixture.load().channels == original.channels);

    // Deleting everything is refused, like trim's.
    CHECK_FALSE(registry.execute(std::make_unique<app::DeleteAudioRegionCommand>(
        fixture.assetId, engine::edits::Region{0, 999999})));
}

TEST_CASE("a trim that would keep nothing, or everything, is refused")
{
    EditFixture fixture;

    app::TrimAssetCommand keepNothing{fixture.assetId, edits::Region{2000, 2000}};
    CHECK_FALSE(keepNothing.execute(fixture.project));

    app::TrimAssetCommand keepEverything{fixture.assetId, edits::Region{0, 5000}};
    CHECK_FALSE(keepEverything.execute(fixture.project));

    CHECK(fixture.load().frameCount == 5000);
}
