// Phase 9b — audio clips become first-class playlist citizens.
//
// The load-bearing property is exact undo of frame-anchored placement: an
// audio clip's position and length live in frames, the grid drags in ticks,
// and tick->frame conversion does not invert exactly — so undo restores
// snapshots, and these tests prove the frames come back identical. The
// merged-drag redo test also pins the fix for replaying a whole gesture.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ClipCommands.h"
#include "project/Model.h"

#include <memory>

using namespace incdaw;
using engine::FrameCount;
using engine::FramePosition;
using engine::Tick;
using engine::ticksPerQuarterNote;

namespace {

struct Fixture {
    project::Project  project;
    project::EntityId audioTrack;
    project::EntityId patternTrack;
    project::EntityId audioClip;
    project::EntityId patternClip;

    Fixture()
    {
        project.tempoMap().setSampleRate(48000.0);

        audioTrack   = project.addTrack(project::TrackType::audio, "Audio").id;
        patternTrack = project.addTrack(project::TrackType::instrument, "Inst").id;

        auto& asset = project.addAudioAsset("/tmp/fake.wav");
        asset.frameCount = 480000;

        auto& audio  = project.addClip(project::ClipType::audio, audioTrack, asset.id);
        audio.start  = 48000;
        audio.length = 96000;
        audioClip    = audio.id;

        auto& pattern = project.addPattern("P1");
        pattern.length = ticksPerQuarterNote * 4;

        auto& clip       = project.addClip(project::ClipType::pattern, patternTrack, pattern.id);
        clip.startTick   = ticksPerQuarterNote;
        clip.lengthTicks = pattern.length;
        patternClip      = clip.id;
    }

    [[nodiscard]] const project::Clip& audio() const { return *project.findClip(audioClip); }
    [[nodiscard]] const project::Clip& pattern() const { return *project.findClip(patternClip); }
};

} // namespace

TEST_CASE("an audio clip moves in frames when dragged in ticks, and undoes exactly")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto& tempoMap = fixture.project.tempoMap();
    const FramePosition expected = tempoMap.frameForTick(
        tempoMap.tickForFrame(48000) + ticksPerQuarterNote);

    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.audioClip}, ticksPerQuarterNote, 0)));

    CHECK(fixture.audio().start == expected);
    CHECK(fixture.audio().length == 96000);   // moves never touch length

    REQUIRE(registry.undo());
    CHECK(fixture.audio().start == 48000);    // the snapshot, bit-exact

    REQUIRE(registry.redo());
    CHECK(fixture.audio().start == expected);
}

TEST_CASE("a mixed selection moves together and clamps as one")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    // The pattern clip sits at one quarter note; asking the pair to move two
    // quarters left must clamp so the EARLIEST clip lands at zero and the
    // group keeps its spacing. The audio clip at 48000 frames (two quarters
    // at 120 bpm) is the earlier one in ticks? One quarter at 120 bpm/48k is
    // 24000 frames, so the audio clip is at two quarters — the pattern clip
    // at one quarter clamps the group to -1 quarter.
    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.audioClip, fixture.patternClip}, -2 * ticksPerQuarterNote, 0)));

    CHECK(fixture.pattern().startTick == 0);
    CHECK(fixture.audio().start == 24000);    // moved by the clamped -1 quarter

    REQUIRE(registry.undo());
    CHECK(fixture.pattern().startTick == ticksPerQuarterNote);
    CHECK(fixture.audio().start == 48000);
}

TEST_CASE("a merged drag undoes to the gesture start and redoes the whole gesture")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    // Two steps of one drag, merged into one history entry — for both clip
    // types at once.
    REQUIRE(registry.executeMerging(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.audioClip, fixture.patternClip}, ticksPerQuarterNote, 0)));
    REQUIRE(registry.executeMerging(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.audioClip, fixture.patternClip}, ticksPerQuarterNote, 0)));

    const FramePosition audioAfter   = fixture.audio().start;
    const Tick          patternAfter = fixture.pattern().startTick;

    CHECK(patternAfter == 3 * ticksPerQuarterNote);
    CHECK(registry.undoDepth() == 1);   // one gesture, one entry

    REQUIRE(registry.undo());
    CHECK(fixture.audio().start == 48000);
    CHECK(fixture.pattern().startTick == ticksPerQuarterNote);

    // The fix under test: redo must replay BOTH steps, not just the first.
    REQUIRE(registry.redo());
    CHECK(fixture.audio().start == audioAfter);
    CHECK(fixture.pattern().startTick == patternAfter);

    REQUIRE(registry.undo());
    CHECK(fixture.audio().start == 48000);
    CHECK(fixture.pattern().startTick == ticksPerQuarterNote);
}

TEST_CASE("an audio clip resizes at its end and undoes exactly")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    const auto& tempoMap = fixture.project.tempoMap();
    const FramePosition end = 48000 + 96000;
    const FrameCount expected = tempoMap.frameForTick(
        tempoMap.tickForFrame(end) + ticksPerQuarterNote) - 48000;

    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(
        app::ClipIds{fixture.audioClip}, ticksPerQuarterNote)));

    CHECK(fixture.audio().length == expected);
    CHECK(fixture.audio().start == 48000);    // resizes never touch position

    REQUIRE(registry.undo());
    CHECK(fixture.audio().length == 96000);

    REQUIRE(registry.redo());
    CHECK(fixture.audio().length == expected);
}

TEST_CASE("shrinking an audio clip below nothing clamps to one frame")
{
    Fixture fixture;
    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::ResizeClipsCommand>(
        app::ClipIds{fixture.audioClip}, -1000 * ticksPerQuarterNote)));

    CHECK(fixture.audio().length == 1);

    REQUIRE(registry.undo());
    CHECK(fixture.audio().length == 96000);
}

TEST_CASE("a pure track move never round-trips the frame position")
{
    Fixture fixture;

    // A second audio track to move onto.
    fixture.project.addTrack(project::TrackType::audio, "Audio 2");

    app::CommandRegistry registry{fixture.project};

    REQUIRE(registry.execute(std::make_unique<app::MoveClipsCommand>(
        app::ClipIds{fixture.audioClip}, 0, 1)));

    CHECK(fixture.audio().start == 48000);    // untouched to the frame

    REQUIRE(registry.undo());
    CHECK(fixture.audio().track == fixture.audioTrack);
    CHECK(fixture.audio().start == 48000);
}
