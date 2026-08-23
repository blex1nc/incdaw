// C8 — comping over recorded takes.
//
// Loop recording stacked passes and stopped there: one file, one clip per
// pass, every pass but the last muted. Comping is the choosing, and it needs
// no new project field — "which take is audible here" is already expressible
// as a split and a mute, which means the composite IS what plays rather than a
// second representation that has to be kept in step with the first.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/RecordingCommands.h"
#include "project/Model.h"

#include <memory>
#include <vector>

using namespace incdaw;

namespace {

/// Three passes over the same bar, from one recording: the shape loop
/// recording produces. Pass N starts N * length frames into the file.
struct TakeStack {
    project::Project  project;
    project::EntityId track;
    project::EntityId asset;

    static constexpr engine::FrameCount passLength = 48000;
    static constexpr int                passCount  = 3;

    TakeStack()
    {
        track = project.addTrack(project::TrackType::audio, "Vocals").id;
        asset = project.addAudioAsset("/takes/vocal.wav").id;

        for (int pass = 0; pass < passCount; ++pass) {
            project::Clip& clip = project.addClip(project::ClipType::audio, track, asset);
            clip.start        = 0;
            clip.length       = passLength;
            clip.sourceOffset = static_cast<engine::FrameCount>(pass) * passLength;

            // Every pass but the last is muted, which is what the recorder
            // leaves behind.
            clip.muted = pass != passCount - 1;
        }
    }

    /// The clips on the track, in timeline order.
    [[nodiscard]] std::vector<project::Clip> clips() const
    {
        std::vector<project::Clip> result;
        for (const project::Clip& clip : project.clips())
            if (clip.track == track)
                result.push_back(clip);

        std::stable_sort(result.begin(), result.end(),
                         [](const project::Clip& left, const project::Clip& right) {
                             if (left.start != right.start)
                                 return left.start < right.start;
                             return left.sourceOffset < right.sourceOffset;
                         });

        return result;
    }

    /// Which take is audible at `frame`, by source offset, or -1 for silence.
    [[nodiscard]] long long audibleAt(engine::FramePosition frame) const
    {
        for (const project::Clip& clip : project.clips()) {
            if (clip.track != track || clip.muted)
                continue;
            if (frame < clip.start || frame >= clip.start + clip.length)
                continue;

            // The pass number: source offset minus how far into the clip we are.
            return (clip.sourceOffset + (frame - clip.start)) / TakeStack::passLength;
        }

        return -1;
    }
};

} // namespace

// ── The lanes ────────────────────────────────────────────────────────────────

TEST_CASE("the stack reads as one lane per pass, in the order they were played")
{
    TakeStack stack;

    const auto takes = app::comping::takesOver(stack.project, stack.track, 0,
                                               TakeStack::passLength);

    REQUIRE(takes.size() == 3);

    // Ordered by anchor, which for loop recording is the order the passes
    // happened — each starts one loop later in the same file. Any other order
    // renumbers the lanes the moment a take is muted.
    CHECK(takes[0].anchor == 0);
    CHECK(takes[1].anchor == TakeStack::passLength);
    CHECK(takes[2].anchor == 2 * TakeStack::passLength);

    CHECK_FALSE(takes[0].audible);
    CHECK_FALSE(takes[1].audible);
    CHECK(takes[2].audible);
}

TEST_CASE("a range outside the takes has no lanes")
{
    TakeStack stack;

    const auto takes = app::comping::takesOver(stack.project, stack.track,
                                               TakeStack::passLength * 4,
                                               TakeStack::passLength * 5);
    CHECK(takes.empty());
}

// ── Assigning ────────────────────────────────────────────────────────────────

TEST_CASE("assigning a range makes one take audible there and the others silent")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    // The first half of the bar comes from pass 1 (the middle lane).
    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 0, TakeStack::passLength / 2, 1)));

    CHECK(stack.audibleAt(0) == 1);
    CHECK(stack.audibleAt(TakeStack::passLength / 2 - 1) == 1);

    // And the untouched half still plays what it played before.
    CHECK(stack.audibleAt(TakeStack::passLength / 2) == 2);
    CHECK(stack.audibleAt(TakeStack::passLength - 1) == 2);
}

TEST_CASE("a split piece plays the right part of the recording")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 12000, 24000, 0)));

    // Every piece must satisfy sourceOffset - start == the take's anchor, or
    // the audio jumps at the seam — the one failure a listener notices
    // immediately and a screenshot never shows.
    for (const project::Clip& clip : stack.clips()) {
        const engine::FrameCount anchor =
            clip.sourceOffset - static_cast<engine::FrameCount>(clip.start);

        CHECK(anchor % TakeStack::passLength == 0);
        CHECK(anchor >= 0);
        CHECK(anchor <= 2 * TakeStack::passLength);
    }
}

TEST_CASE("two assignments compose rather than replacing each other")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 0, 16000, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 16000, 32000, 1)));

    // Comping one span must not undo the span beside it. A comp is built by
    // accumulation; an editor that forgot the last decision on every new one
    // would be unusable.
    CHECK(stack.audibleAt(0) == 0);
    CHECK(stack.audibleAt(15999) == 0);
    CHECK(stack.audibleAt(16000) == 1);
    CHECK(stack.audibleAt(31999) == 1);
    CHECK(stack.audibleAt(32000) == 2);
}

TEST_CASE("exactly one take is audible at every frame of a comped range")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 5000, 20000, 0)));

    for (engine::FramePosition frame = 0; frame < TakeStack::passLength; frame += 997) {
        int audible = 0;

        for (const project::Clip& clip : stack.project.clips()) {
            if (clip.muted || clip.track != stack.track)
                continue;
            if (frame >= clip.start && frame < clip.start + clip.length)
                ++audible;
        }

        // Two audible takes over one frame is two vocals at once, which is
        // what comping exists to stop.
        CHECK(audible == 1);
    }
}

// ── Undo ─────────────────────────────────────────────────────────────────────

TEST_CASE("one undo entry per assignment, and it peels exactly that decision")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    const auto original = stack.clips();

    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 0, 16000, 0)));
    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 16000, 32000, 1)));

    registry.undo();
    CHECK(stack.audibleAt(16000) == 2);   // the second decision, gone
    CHECK(stack.audibleAt(0) == 0);       // the first, still there

    registry.undo();
    CHECK(stack.clips() == original);

    registry.redo();
    registry.redo();
    CHECK(stack.audibleAt(16000) == 1);
}

// ── Refusals ─────────────────────────────────────────────────────────────────

TEST_CASE("assigning the take that is already audible makes no undo entry")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    const std::size_t depth = registry.undoDepth();

    // Pass 2 is what the recorder left audible. Choosing it again changes
    // nothing, and an undo entry that changes nothing is worse than none.
    CHECK_FALSE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, 0, TakeStack::passLength, 2)));

    CHECK(registry.undoDepth() == depth);
}

TEST_CASE("an empty range and a lane that does not exist are both refused")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    CHECK_FALSE(registry.execute(
        std::make_unique<app::AssignCompRangeCommand>(stack.track, 1000, 1000, 0)));
    CHECK_FALSE(registry.execute(
        std::make_unique<app::AssignCompRangeCommand>(stack.track, 2000, 1000, 0)));
    CHECK_FALSE(registry.execute(
        std::make_unique<app::AssignCompRangeCommand>(stack.track, 0, 1000, 9)));
}

TEST_CASE("a range wider than the takes still comps the part that overlaps")
{
    TakeStack stack;
    app::CommandRegistry registry{stack.project};

    REQUIRE(registry.execute(std::make_unique<app::AssignCompRangeCommand>(
        stack.track, -10000, TakeStack::passLength * 2, 0)));

    CHECK(stack.audibleAt(0) == 0);
    CHECK(stack.audibleAt(TakeStack::passLength - 1) == 0);

    // Nothing was invented beyond where the takes actually are.
    for (const project::Clip& clip : stack.clips()) {
        CHECK(clip.start >= 0);
        CHECK(clip.start + clip.length <= TakeStack::passLength);
    }
}
