// Phase 12 — loop and punch recording.
//
// The capture is one continuous file; placement cuts it against the loop it
// was recorded under. The load-bearing properties: every file frame lands
// exactly once (before punching), passes stack with only the newest audible,
// punching trims placements without corrupting file offsets, and the
// multi-clip landing undoes as one.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/RecordingCommands.h"
#include "project/Model.h"
#include "project/RecordingSession.h"

#include <memory>

using namespace incdaw;
using engine::FrameCount;
using engine::FramePosition;
using project::RecordingSession;

TEST_CASE("a take that never wraps is one straight slice")
{
    RecordingSession::TakeGeometry geometry;
    geometry.takeFrames  = 10000;
    geometry.takeStart   = 5000;
    geometry.loopStart   = 0;
    geometry.loopEnd     = 48000;
    geometry.loopEnabled = true;

    const auto slices = RecordingSession::computeSlices(geometry);

    REQUIRE(slices.size() == 1);
    CHECK(slices[0].startFrame == 5000);
    CHECK(slices[0].sourceOffset == 0);
    CHECK(slices[0].length == 10000);
    CHECK_FALSE(slices[0].muted);
}

TEST_CASE("a wrapped take becomes one slice per pass, older passes muted")
{
    RecordingSession::TakeGeometry geometry;
    geometry.takeFrames  = 100000;      // 2 full passes + a bit, from mid-loop
    geometry.takeStart   = 10000;
    geometry.loopStart   = 0;
    geometry.loopEnd     = 48000;
    geometry.loopEnabled = true;

    const auto slices = RecordingSession::computeSlices(geometry);

    REQUIRE(slices.size() == 3);

    // Pass 1: from the arm point to the loop end.
    CHECK(slices[0].startFrame == 10000);
    CHECK(slices[0].sourceOffset == 0);
    CHECK(slices[0].length == 38000);
    CHECK(slices[0].muted);

    // Pass 2: a full loop.
    CHECK(slices[1].startFrame == 0);
    CHECK(slices[1].sourceOffset == 38000);
    CHECK(slices[1].length == 48000);
    CHECK(slices[1].muted);

    // Pass 3: the remainder — the newest, audible.
    CHECK(slices[2].startFrame == 0);
    CHECK(slices[2].sourceOffset == 86000);
    CHECK(slices[2].length == 14000);
    CHECK_FALSE(slices[2].muted);

    // Every file frame lands exactly once.
    FrameCount total = 0;
    for (const auto& slice : slices)
        total += slice.length;
    CHECK(total == geometry.takeFrames);
}

TEST_CASE("a take armed before the loop runs linearly into it, then wraps")
{
    RecordingSession::TakeGeometry geometry;
    geometry.takeFrames  = 60000;
    geometry.takeStart   = 8000;        // before the loop's start
    geometry.loopStart   = 24000;
    geometry.loopEnd     = 48000;
    geometry.loopEnabled = true;

    const auto slices = RecordingSession::computeSlices(geometry);

    REQUIRE(slices.size() == 2);
    CHECK(slices[0].startFrame == 8000);
    CHECK(slices[0].length == 40000);   // straight through to the loop end
    CHECK(slices[1].startFrame == 24000);
    CHECK(slices[1].sourceOffset == 40000);
    CHECK(slices[1].length == 20000);
}

TEST_CASE("punch trims every slice and keeps file offsets honest")
{
    RecordingSession::TakeGeometry geometry;
    geometry.takeFrames  = 96000;       // two full passes
    geometry.takeStart   = 0;
    geometry.loopStart   = 0;
    geometry.loopEnd     = 48000;
    geometry.loopEnabled = true;
    geometry.punchIn     = 10000;
    geometry.punchOut    = 20000;

    const auto slices = RecordingSession::computeSlices(geometry);

    REQUIRE(slices.size() == 2);

    CHECK(slices[0].startFrame == 10000);
    CHECK(slices[0].sourceOffset == 10000);
    CHECK(slices[0].length == 10000);
    CHECK(slices[0].muted);

    // Second pass: file frame 48000 sits at timeline 0, so the punch window
    // holds file frames 58000..68000.
    CHECK(slices[1].startFrame == 10000);
    CHECK(slices[1].sourceOffset == 58000);
    CHECK(slices[1].length == 10000);
    CHECK_FALSE(slices[1].muted);
}

TEST_CASE("a punch window the take never enters yields nothing")
{
    RecordingSession::TakeGeometry geometry;
    geometry.takeFrames  = 5000;
    geometry.takeStart   = 0;
    geometry.loopStart   = 0;
    geometry.loopEnd     = 48000;
    geometry.loopEnabled = true;
    geometry.punchIn     = 40000;
    geometry.punchOut    = 44000;

    CHECK(RecordingSession::computeSlices(geometry).empty());
}

TEST_CASE("a sliced take lands one clip per pass and undoes as one")
{
    project::Project projectModel;
    app::CommandRegistry registry{projectModel};

    RecordingSession::Placement placement;
    placement.succeeded    = true;
    placement.path         = "/tmp/loop-take.wav";
    placement.frameCount   = 100000;
    placement.channelCount = 1;
    placement.sampleRate   = 48000.0;
    placement.sliced       = true;
    placement.slices       = {
        {10000, 0, 38000, true},
        {0, 38000, 48000, true},
        {0, 86000, 14000, false},
    };
    placement.startFrame = placement.slices.front().startFrame;

    REQUIRE(registry.execute(std::make_unique<app::InsertRecordedTakeCommand>(placement)));

    REQUIRE(projectModel.clips().size() == 3);
    CHECK(projectModel.clips()[0].muted);
    CHECK(projectModel.clips()[1].muted);
    CHECK_FALSE(projectModel.clips()[2].muted);
    CHECK(projectModel.clips()[1].sourceOffset == 38000);
    CHECK(projectModel.clips()[2].name.find("#3") != std::string::npos);

    // All three share the one asset — one recording, three views of it.
    for (const auto& clip : projectModel.clips())
        CHECK(clip.source == projectModel.audioAssets().front().id);

    const auto firstClipId = projectModel.clips().front().id;

    REQUIRE(registry.undo());
    CHECK(projectModel.clips().empty());
    CHECK(projectModel.audioAssets().empty());
    CHECK(projectModel.tracks().empty());

    REQUIRE(registry.redo());
    REQUIRE(projectModel.clips().size() == 3);
    CHECK(projectModel.clips().front().id == firstClipId);
}

TEST_CASE("a sliced-but-empty placement lands nothing at all")
{
    project::Project projectModel;

    RecordingSession::Placement placement;
    placement.succeeded    = true;
    placement.path         = "/tmp/loop-empty.wav";
    placement.frameCount   = 5000;
    placement.channelCount = 1;
    placement.sampleRate   = 48000.0;
    placement.sliced       = true;      // punch excluded everything

    app::InsertRecordedTakeCommand command{placement};
    CHECK_FALSE(command.execute(projectModel));
    CHECK(projectModel.clips().empty());
    CHECK(projectModel.audioAssets().empty());
}
