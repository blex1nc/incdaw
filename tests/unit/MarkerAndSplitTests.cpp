#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/MarkerCommands.h"
#include "project/PatternCompiler.h"
#include "project/ProjectFile.h"

#include <atomic>
#include <filesystem>
#include <memory>

using namespace incdaw;
using namespace incdaw::app;
using incdaw::engine::ticksPerQuarterNote;

namespace fs = std::filesystem;

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-test-" + name + "-" + std::to_string(nextSerial())))
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
        static std::atomic<int> counter{0};
        return ++counter;
    }
};

project::MidiEvent note(Tick tick, int key, Tick duration = 240)
{
    project::MidiEvent event;
    event.type     = project::MidiEventType::note;
    event.tick     = tick;
    event.key      = key;
    event.duration = duration;
    return event;
}

/// A project with one channel, one two-bar pattern, one track, and one
/// pattern clip placed at bar 2 — enough arrangement to cut something.
struct SplitFixture {
    project::Project project;
    CommandRegistry  registry { project };
    project::EntityId channel;
    project::EntityId pattern;
    project::EntityId track;
    project::EntityId clip;

    SplitFixture()
    {
        channel = project.addChannel("Synth").id;

        project::Pattern& created = project.addPattern("P1");
        created.length            = ticksPerQuarterNote * 8;
        created.contentFor(channel).events.push_back(note(0, 60));
        created.contentFor(channel).events.push_back(note(ticksPerQuarterNote * 6, 72));
        pattern = created.id;

        track = project.addTrack(project::TrackType::instrument, "Track 1").id;

        project::Clip& placed = project.addClip(project::ClipType::pattern, track, pattern);
        placed.startTick      = ticksPerQuarterNote * 4;
        placed.lengthTicks    = ticksPerQuarterNote * 8;
        clip                  = placed.id;
    }
};

} // namespace

// ── Clip splitting ────────────────────────────────────────────────────────────

TEST_CASE("splitting a pattern clip yields two halves that continue the source")
{
    SplitFixture fixture;

    const Tick cut = ticksPerQuarterNote * 8;   // one bar into the clip

    auto  command = std::make_unique<SplitClipCommand>(fixture.clip, cut);
    auto* raw     = command.get();
    REQUIRE(fixture.registry.execute(std::move(command)));

    REQUIRE(fixture.project.clips().size() == 2);

    const project::Clip* left  = fixture.project.findClip(fixture.clip);
    const project::Clip* right = fixture.project.findClip(raw->rightClipId());
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    CHECK(left->startTick == ticksPerQuarterNote * 4);
    CHECK(left->lengthTicks == ticksPerQuarterNote * 4);
    CHECK(left->sourceOffsetTicks == 0);

    CHECK(right->startTick == cut);
    CHECK(right->lengthTicks == ticksPerQuarterNote * 4);
    CHECK(right->sourceOffsetTicks == ticksPerQuarterNote * 4);
    CHECK(right->track == left->track);
    CHECK(right->source == left->source);
}

TEST_CASE("a split arrangement compiles to the same notes as the unsplit one")
{
    SplitFixture fixture;

    const std::vector<engine::SequencedNote> before =
        project::compileArrangement(fixture.project, fixture.channel);

    REQUIRE(fixture.registry.execute(std::make_unique<SplitClipCommand>(
        fixture.clip, ticksPerQuarterNote * 8)));

    const std::vector<engine::SequencedNote> after =
        project::compileArrangement(fixture.project, fixture.channel);

    REQUIRE(before.size() == after.size());
    for (std::size_t index = 0; index < before.size(); ++index) {
        CHECK(before[index].startTick == after[index].startTick);
        CHECK(before[index].key == after[index].key);
        CHECK(before[index].lengthTicks == after[index].lengthTicks);
    }
}

TEST_CASE("splitting an audio clip is frame-exact and advances the source offset")
{
    SplitFixture fixture;

    project::AudioAsset& asset = fixture.project.addAudioAsset("media/take.wav");
    asset.sampleRate           = 48000.0;
    asset.frameCount           = 96000;
    asset.channelCount         = 2;

    project::Clip& audio = fixture.project.addClip(project::ClipType::audio,
                                                   fixture.track, asset.id);
    audio.start        = 24000;   // one beat at 120 BPM / 48 kHz
    audio.length       = 48000;   // two beats
    audio.sourceOffset = 1000;
    audio.fadeInFrames  = 300;
    audio.fadeOutFrames = 400;
    const project::EntityId audioId = audio.id;

    // Beat 2 of the timeline is frame 48000 — the middle of the clip.
    auto  command = std::make_unique<SplitClipCommand>(audioId, ticksPerQuarterNote * 2);
    auto* raw     = command.get();
    REQUIRE(fixture.registry.execute(std::move(command)));

    const project::Clip* left  = fixture.project.findClip(audioId);
    const project::Clip* right = fixture.project.findClip(raw->rightClipId());
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    CHECK(left->start == 24000);
    CHECK(left->length == 24000);
    CHECK(left->sourceOffset == 1000);
    CHECK(left->fadeInFrames == 300);
    CHECK(left->fadeOutFrames == 0);      // the cut has no fade

    CHECK(right->start == 48000);
    CHECK(right->length == 24000);
    CHECK(right->sourceOffset == 25000);  // 1000 + the left half's 24000 frames
    CHECK(right->fadeInFrames == 0);
    CHECK(right->fadeOutFrames == 400);
}

TEST_CASE("undoing a split restores the exact original and removes the right half")
{
    SplitFixture fixture;

    const project::Clip original = *fixture.project.findClip(fixture.clip);

    auto  command = std::make_unique<SplitClipCommand>(fixture.clip, ticksPerQuarterNote * 8);
    auto* raw     = command.get();
    REQUIRE(fixture.registry.execute(std::move(command)));
    const project::EntityId rightId = raw->rightClipId();

    CHECK(fixture.registry.undo());
    REQUIRE(fixture.project.clips().size() == 1);
    CHECK(*fixture.project.findClip(fixture.clip) == original);

    // Redo recreates the right half under the same id.
    CHECK(fixture.registry.redo());
    CHECK(fixture.project.findClip(rightId) != nullptr);
}

TEST_CASE("a cut outside the clip is refused")
{
    SplitFixture fixture;

    CHECK_FALSE(fixture.registry.execute(std::make_unique<SplitClipCommand>(
        fixture.clip, ticksPerQuarterNote * 4)));   // exactly at the start
    CHECK_FALSE(fixture.registry.execute(std::make_unique<SplitClipCommand>(
        fixture.clip, ticksPerQuarterNote * 12)));  // exactly at the end
    CHECK_FALSE(fixture.registry.canUndo());
}

// ── Markers ───────────────────────────────────────────────────────────────────

TEST_CASE("markers and regions land, undo and redo with stable ids")
{
    project::Project project;
    CommandRegistry  registry { project };

    auto  command = std::make_unique<AddMarkerCommand>(ticksPerQuarterNote * 16, "Drop");
    auto* raw     = command.get();
    REQUIRE(registry.execute(std::move(command)));
    const project::EntityId id = raw->markerId();

    REQUIRE(registry.execute(std::make_unique<AddMarkerCommand>(
        ticksPerQuarterNote * 32, "Chorus", ticksPerQuarterNote * 16)));

    REQUIRE(project.markers().size() == 2);
    CHECK(project.markers()[0].length == 0);
    CHECK(project.markers()[1].length == ticksPerQuarterNote * 16);

    CHECK(registry.undo());
    CHECK(registry.undo());
    CHECK(project.markers().empty());

    CHECK(registry.redo());
    CHECK(project.findMarker(id) != nullptr);
}

TEST_CASE("merged marker edits are one undo back to the original")
{
    project::Project project;
    CommandRegistry  registry { project };

    project::TimelineMarker& marker = project.addMarker(0, "Intro");
    const project::EntityId  id     = marker.id;

    project::TimelineMarker moved = marker;
    moved.tick                    = ticksPerQuarterNote * 4;
    REQUIRE(registry.executeMerging(std::make_unique<EditMarkerCommand>(id, moved)));

    moved.tick = ticksPerQuarterNote * 8;
    moved.name = "Verse";
    REQUIRE(registry.executeMerging(std::make_unique<EditMarkerCommand>(id, moved)));

    CHECK(project.findMarker(id)->tick == ticksPerQuarterNote * 8);
    CHECK(project.findMarker(id)->name == "Verse");
    CHECK(registry.undoDepth() == 1);

    CHECK(registry.undo());
    CHECK(project.findMarker(id)->tick == 0);
    CHECK(project.findMarker(id)->name == "Intro");
}

TEST_CASE("removing a marker restores at its position on undo")
{
    project::Project project;
    CommandRegistry  registry { project };

    project.addMarker(0, "A");
    const project::EntityId middle = project.addMarker(100, "B").id;
    project.addMarker(200, "C");

    REQUIRE(registry.execute(std::make_unique<RemoveMarkerCommand>(middle)));
    REQUIRE(project.markers().size() == 2);

    CHECK(registry.undo());
    REQUIRE(project.markers().size() == 3);
    CHECK(project.markers()[1].id == middle);
    CHECK(project.markers()[1].name == "B");
}

TEST_CASE("the v1.5 fixture still loads")
{
    const fs::path fixture = fs::path{INCDAW_FIXTURE_DIR} / "v1.5" / "Fixture.incdaw";
    REQUIRE(fs::exists(fixture));

    project::Project project;
    const auto result = project::ProjectFile::load(project, fixture);
    REQUIRE(result);

    CHECK(project.metadata().title == "Format v1.5 fixture");

    REQUIRE(project.markers().size() == 2);
    CHECK(project.markers()[0].name == "Drop");
    CHECK(project.markers()[0].tick == 3840);
    CHECK(project.markers()[0].length == 0);
    CHECK(project.markers()[1].name == "Chorus");
    CHECK(project.markers()[1].length == 3840);
}

TEST_CASE("markers round-trip through the project file")
{
    ScratchDirectory scratch("markers");

    project::Project saved;
    saved.metadata().title = "Marker round-trip";
    saved.addMarker(ticksPerQuarterNote * 16, "Drop");
    project::TimelineMarker& region = saved.addMarker(ticksPerQuarterNote * 32, "Chorus");
    region.length                   = ticksPerQuarterNote * 16;
    region.colour                   = 0xFF223344u;

    const fs::path file = scratch.path / "Markers.incdaw";
    REQUIRE(project::ProjectFile::save(saved, file));

    project::Project loaded;
    REQUIRE(project::ProjectFile::load(loaded, file));

    REQUIRE(loaded.markers().size() == 2);
    CHECK(loaded.markers()[0].name == "Drop");
    CHECK(loaded.markers()[1].length == ticksPerQuarterNote * 16);
    CHECK(loaded.markers()[1].colour == 0xFF223344u);
    CHECK(loaded == saved);
}
