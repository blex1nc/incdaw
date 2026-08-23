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

// ── C7: markers and regions live in the file ─────────────────────────────────
//
// Stored as RIFF's own cue/adtl chunks rather than in the project, so a file
// marked up here opens marked up elsewhere — and so INCDAW stops destroying
// the cues other editors wrote, which it did on every read-edit-write until
// the writer learned to emit them.

namespace {

engine::AudioMarker marker(std::string name, engine::FramePosition start,
                           engine::FrameCount length = 0)
{
    engine::AudioMarker result;
    result.name   = std::move(name);
    result.start  = start;
    result.length = length;
    return result;
}

} // namespace

TEST_CASE("markers survive a write and a read")
{
    ScratchDir scratch{"incdaw-markers"};
    const auto file = scratch.path / "marked.wav";

    auto data = makeAudio(2, 4000);
    data.markers = {marker("Intro", 0), marker("Verse", 1000, 500), marker("Drop", 3000)};

    REQUIRE(bool(WavFile::write(file, data)));

    AudioFileData reloaded;
    REQUIRE(bool(WavFile::read(file, reloaded)));

    REQUIRE(reloaded.markers.size() == 3);
    CHECK(reloaded.markers == data.markers);

    // A region keeps its length; a point keeps its zero.
    CHECK(reloaded.markers[1].isRegion());
    CHECK(reloaded.markers[1].end() == 1500);
    CHECK_FALSE(reloaded.markers[0].isRegion());
}

TEST_CASE("an odd-length marker name does not shift the chunks after it")
{
    ScratchDir scratch{"incdaw-markerpad"};
    const auto file = scratch.path / "odd.wav";

    // "Hat" is three characters, four with its terminator — an even body — so
    // the one that must be padded is the five-character name below it. Getting
    // the pad wrong moves every chunk after it by a byte, which a reader sees
    // as garbage rather than as a missing label.
    auto data = makeAudio(1, 100);
    data.markers = {marker("Hats", 10), marker("Snare", 20), marker("Kick", 30)};

    REQUIRE(bool(WavFile::write(file, data)));

    AudioFileData reloaded;
    REQUIRE(bool(WavFile::read(file, reloaded)));

    REQUIRE(reloaded.markers.size() == 3);
    CHECK(reloaded.markers[0].name == "Hats");
    CHECK(reloaded.markers[1].name == "Snare");
    CHECK(reloaded.markers[2].name == "Kick");
}

TEST_CASE("a file with no markers is byte-identical to one written before they existed")
{
    ScratchDir scratch{"incdaw-nomarkers"};
    const auto file = scratch.path / "plain.wav";

    const auto data = makeAudio(2, 256);
    REQUIRE(bool(WavFile::write(file, data)));

    // No cue chunk is written at all, so the canonical 44-byte header plus the
    // samples is the whole file — which is what the streaming writer patches
    // by fixed offset and what every other tool expects.
    CHECK(fs::file_size(file) == 44u + 256u * 2u * 4u);
}

TEST_CASE("markers are read back sorted, whatever order they were written in")
{
    ScratchDir scratch{"incdaw-markersort"};
    const auto file = scratch.path / "unsorted.wav";

    auto data = makeAudio(1, 1000);
    data.markers = {marker("Last", 900), marker("First", 10), marker("Middle", 400)};

    REQUIRE(bool(WavFile::write(file, data)));

    AudioFileData reloaded;
    REQUIRE(bool(WavFile::read(file, reloaded)));

    REQUIRE(reloaded.markers.size() == 3);
    CHECK(reloaded.markers[0].name == "First");
    CHECK(reloaded.markers[1].name == "Middle");
    CHECK(reloaded.markers[2].name == "Last");
}

// ── Coherence under the edit verbs ───────────────────────────────────────────

TEST_CASE("inserting audio pushes the markers after it")
{
    auto data = makeAudio(1, 100);
    data.markers = {marker("A", 10), marker("B", 50), marker("C", 90)};

    const auto piece = engine::edits::extractRegion(data, {0, 20});
    REQUIRE(engine::edits::insertAudio(data, 50, piece));

    REQUIRE(data.markers.size() == 3);
    CHECK(data.markers[0].start == 10);   // before the insertion: unmoved
    CHECK(data.markers[1].start == 70);   // exactly on it: moves with the sound after it
    CHECK(data.markers[2].start == 110);
}

TEST_CASE("a copied span carries no markers")
{
    auto data = makeAudio(1, 100);
    data.markers = {marker("A", 10), marker("B", 50)};

    // Pasting a span should add sound, not somebody's annotations.
    const auto piece = engine::edits::extractRegion(data, {0, 60});
    CHECK(piece.markers.empty());
}

TEST_CASE("deleting audio takes the markers under it")
{
    auto data = makeAudio(1, 100);
    data.markers = {marker("Before", 10), marker("Inside", 45), marker("After", 80)};

    engine::edits::deleteRegion(data, {40, 60});

    REQUIRE(data.markers.size() == 2);
    CHECK(data.markers[0].name == "Before");
    CHECK(data.markers[0].start == 10);

    // The one that named the removed sound went with it; the one after closed
    // up by exactly what was taken out.
    CHECK(data.markers[1].name == "After");
    CHECK(data.markers[1].start == 60);
}

TEST_CASE("a region marker keeps whatever survived a deletion")
{
    auto data = makeAudio(1, 100);
    data.markers = {marker("Straddles", 30, 40)};   // [30, 70)

    engine::edits::deleteRegion(data, {50, 60});

    REQUIRE(data.markers.size() == 1);
    CHECK(data.markers[0].start == 30);
    CHECK(data.markers[0].length == 30);   // lost the ten frames it overlapped

    // Removing all of it is the one case where the annotation goes too.
    engine::edits::deleteRegion(data, {25, 70});
    CHECK(data.markers.empty());
}

TEST_CASE("trim rebases what it kept and drops what it did not")
{
    auto data = makeAudio(1, 100);
    data.markers = {marker("Head", 5), marker("Kept", 40), marker("Span", 45, 20),
                    marker("Tail", 95)};

    engine::edits::trimTo(data, {30, 70});

    REQUIRE(data.markers.size() == 2);
    CHECK(data.markers[0].name == "Kept");
    CHECK(data.markers[0].start == 10);

    CHECK(data.markers[1].name == "Span");
    CHECK(data.markers[1].start == 15);
    CHECK(data.markers[1].length == 20);
}

TEST_CASE("the verbs that do not change length leave markers alone")
{
    auto data = makeAudio(1, 100);
    const std::vector<engine::AudioMarker> before{marker("A", 10), marker("B", 60, 20)};
    data.markers = before;

    engine::edits::applyGain(data, {0, 100}, 0.5f);
    engine::edits::reverse(data, {20, 80});
    engine::edits::fadeIn(data, {0, 10});
    engine::edits::silence(data, {90, 100});

    // Reversing does NOT mirror the markers inside it: a marker names a moment
    // in the material, and the material is what was reversed.
    CHECK(data.markers == before);
}

// ── Undo ─────────────────────────────────────────────────────────────────────

TEST_CASE("undo restores markers an edit removed")
{
    EditFixture fixture;
    app::CommandRegistry registry{fixture.project};

    {
        auto data = fixture.load();
        data.markers = {marker("Before", 100), marker("Inside", 1500), marker("After", 3000)};
        REQUIRE(bool(WavFile::write(fixture.file, data)));
    }

    REQUIRE(registry.execute(std::make_unique<app::DeleteAudioRegionCommand>(
        fixture.assetId, engine::edits::Region{1000, 2000})));

    {
        const auto data = fixture.load();
        REQUIRE(data.markers.size() == 2);
        CHECK(data.markers[1].start == 2000);   // "After", closed up
    }

    // The marker inside the deleted span cannot be derived from the two that
    // are left, which is why the command snapshots the list rather than trying
    // to un-shift it.
    registry.undo();

    const auto restored = fixture.load();
    REQUIRE(restored.markers.size() == 3);
    CHECK(restored.markers[1].name == "Inside");
    CHECK(restored.markers[1].start == 1500);

    registry.redo();
    CHECK(fixture.load().markers.size() == 2);
}

TEST_CASE("a trim's undo brings back the markers outside what it kept")
{
    EditFixture fixture;
    app::CommandRegistry registry{fixture.project};

    {
        auto data = fixture.load();
        data.markers = {marker("Head", 10), marker("Kept", 2500), marker("Tail", 4900)};
        REQUIRE(bool(WavFile::write(fixture.file, data)));
    }

    REQUIRE(registry.execute(std::make_unique<app::TrimAssetCommand>(
        fixture.assetId, engine::edits::Region{2000, 3000})));

    CHECK(fixture.load().markers.size() == 1);

    registry.undo();

    const auto restored = fixture.load();
    REQUIRE(restored.markers.size() == 3);
    CHECK(restored.markers[0].name == "Head");
    CHECK(restored.markers[2].name == "Tail");
}

// ── C6: the clipboard crosses files ──────────────────────────────────────────

TEST_CASE("a span copied from one asset pastes into another")
{
    ScratchDir scratch{"incdaw-crossfile"};

    project::Project project;

    const auto sourceFile = scratch.path / "source.wav";
    const auto targetFile = scratch.path / "target.wav";

    const auto source = makeAudio(2, 2000);
    auto       target = makeAudio(2, 1000);
    for (auto& channel : target.channels)
        std::fill(channel.begin(), channel.end(), 0.0f);

    REQUIRE(bool(WavFile::write(sourceFile, source)));
    REQUIRE(bool(WavFile::write(targetFile, target)));

    auto& targetAsset       = project.addAudioAsset(targetFile.string());
    targetAsset.sampleRate  = 48000.0;
    targetAsset.frameCount  = 1000;
    targetAsset.channelCount = 2;

    // What the shell's clipboard holds: a span lifted out of one file, with no
    // tie to the asset it came from. That is what lets it survive a document
    // switch and land in a different one.
    const auto clipboard = engine::edits::extractRegion(source, {500, 700});
    REQUIRE(clipboard.frameCount == 200);

    app::CommandRegistry registry{project};
    REQUIRE(registry.execute(
        std::make_unique<app::InsertAudioCommand>(targetAsset.id, 400, clipboard)));

    AudioFileData pasted;
    REQUIRE(bool(WavFile::read(targetFile, pasted)));
    CHECK(pasted.frameCount == 1200);
    CHECK(pasted.channels[0][400] == doctest::Approx(source.channels[0][500]));
    CHECK(pasted.channels[1][599] == doctest::Approx(source.channels[1][699]));

    // And the target's own audio is intact either side of the seam.
    CHECK(pasted.channels[0][399] == doctest::Approx(0.0));
    CHECK(pasted.channels[0][600] == doctest::Approx(0.0));

    registry.undo();

    AudioFileData undone;
    REQUIRE(bool(WavFile::read(targetFile, undone)));
    CHECK(undone.frameCount == 1000);
}

TEST_CASE("a paste from a file at another rate is refused, not resampled")
{
    ScratchDir scratch{"incdaw-ratemismatch"};

    project::Project project;
    const auto file = scratch.path / "target.wav";

    const auto target = makeAudio(2, 500);
    REQUIRE(bool(WavFile::write(file, target)));

    auto& asset        = project.addAudioAsset(file.string());
    asset.sampleRate   = 48000.0;
    asset.frameCount   = 500;
    asset.channelCount = 2;

    auto clipboard        = engine::edits::extractRegion(target, {0, 100});
    clipboard.sampleRate  = 44100.0;

    // Silently resampling would change the pitch of the pasted sound; silently
    // not resampling would change its speed. Refusing says so.
    app::CommandRegistry registry{project};
    CHECK_FALSE(registry.execute(
        std::make_unique<app::InsertAudioCommand>(asset.id, 0, clipboard)));

    AudioFileData unchanged;
    REQUIRE(bool(WavFile::read(file, unchanged)));
    CHECK(unchanged.frameCount == 500);
}
