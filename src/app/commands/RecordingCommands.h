#pragma once

#include "app/Command.h"
#include "project/Model.h"
#include "project/RecordingSession.h"

namespace incdaw::app {

/// Lands a finished take in the project: the file becomes an AudioAsset, the
/// asset becomes an audio clip at the take's latency-compensated position, on
/// the first audio track — created on the spot when the project has none.
///
/// One command for all of it so that one Cmd+Z removes the take from the
/// arrangement whole: clip, asset, and the track if this command created it.
/// The FILE on disk is deliberately left alone — undo removes the take from
/// the project, it does not delete a recording the user may want back.
class InsertRecordedTakeCommand final : public Command {
public:
    explicit InsertRecordedTakeCommand(project::RecordingSession::Placement placement)
        : placement_(std::move(placement)) {}

    [[nodiscard]] const char* id() const noexcept override { return "recording.insertTake"; }
    [[nodiscard]] std::string name() const override { return "Record Take"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// The first landed clip, valid only after the first execute.
    [[nodiscard]] project::EntityId clipId() const noexcept
    {
        return clips_.empty() ? project::EntityId{} : clips_.front().id;
    }

private:
    project::RecordingSession::Placement placement_;

    project::AudioAsset asset_;

    /// One clip per placed slice; a straight take has exactly one. Kept with
    /// their insertion indices so redo restores identical ids and positions.
    std::vector<project::Clip> clips_;
    std::vector<std::size_t>   clipIndices_;

    project::Track      track_;         ///< only meaningful when trackCreated_
    std::size_t         assetIndex_ = 0;
    std::size_t         trackIndex_ = 0;
    bool                trackCreated_ = false;
    bool                minted_       = false;
};

// ── Comping ───────────────────────────────────────────────────────────────────
//
// Loop recording stacks passes: one file, one clip per pass, every pass but
// the last muted (project/RecordingSession.h). That is a pile of takes, not a
// comp — and a pile is where the workflow stopped.
//
// Comping is the choosing. It needs no new project field, because "which take
// is audible here" is already expressible: split the stack at the boundaries
// of the range being assigned, and mute everything in that range except the
// chosen take. The composite is therefore what plays, immediately, with no
// second representation to keep in step with the first.

namespace comping {

/// One take in a stack — the clips that share a source mapping.
///
/// `anchor` is `sourceOffset - start`: the constant that turns a timeline
/// frame into a frame of the recording. It is what identifies a take across
/// the splits comping makes, where a clip id cannot: assigning a range splits
/// a clip into two clips of the SAME take.
struct Take {
    project::EntityId     source;
    engine::FrameCount    anchor = 0;

    engine::FramePosition start = 0;   ///< earliest frame this take covers
    engine::FramePosition end   = 0;   ///< one past the latest

    /// True while any of the take's clips inside the queried range is audible.
    bool audible = false;

    [[nodiscard]] friend bool operator==(const Take&, const Take&) = default;
};

/// The takes on `track` overlapping [from, to), as lanes.
///
/// Ordered by anchor — which for loop recording is the order the passes were
/// played, because each pass starts one loop later in the same file. That is
/// the order a comping editor has to show them in; anything else renumbers the
/// lanes when a take is muted.
[[nodiscard]] std::vector<Take> takesOver(const Project& project, project::EntityId track,
                                          engine::FramePosition from, engine::FramePosition to);

} // namespace comping

/// Makes one take audible over one range, and the others silent there.
///
/// One undo entry per assignment, which is the unit a comp is built in: drag,
/// listen, drag again, and Cmd+Z peels exactly the last decision.
///
/// The whole track's clip list is snapshotted and restored. The operation
/// splits clips and mutes them, and neither is invertible on its own — undoing
/// a split by merging would have to know which two clips were once one, and
/// after a second assignment they may not be adjacent any more.
class AssignCompRangeCommand final : public Command {
public:
    AssignCompRangeCommand(project::EntityId track, engine::FramePosition from,
                           engine::FramePosition to, std::size_t takeIndex)
        : track_(track), from_(from), to_(to), takeIndex_(takeIndex) {}

    [[nodiscard]] const char* id() const noexcept override { return "recording.assignComp"; }
    [[nodiscard]] std::string name() const override { return "Assign Take"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    project::EntityId     track_;
    engine::FramePosition from_ = 0;
    engine::FramePosition to_   = 0;
    std::size_t           takeIndex_ = 0;

    std::vector<project::Clip> before_;
    std::vector<project::Clip> after_;
    bool                       minted_ = false;
};

} // namespace incdaw::app
