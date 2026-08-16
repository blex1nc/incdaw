#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::Clip;
using project::EntityId;
using project::Tick;

/// Playlist clip edits.
///
/// Clips are addressed by id rather than by index, unlike notes: the playlist
/// holds every track's clips in one vector, so an edit on one track can move
/// the indices of another's. Ids are stable across that, and across undo.
using ClipIds = std::vector<EntityId>;

/// Places a pattern on a track.
class AddPatternClipCommand final : public Command {
public:
    AddPatternClipCommand(EntityId track, EntityId pattern, Tick startTick, Tick lengthTicks = 0)
        : track_(track), pattern_(pattern), start_(startTick), length_(lengthTicks) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.addPattern"; }
    [[nodiscard]] std::string name() const override { return "Add Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId clipId() const noexcept { return clip_.id; }

private:
    EntityId    track_;
    EntityId    pattern_;
    Tick        start_  = 0;
    Tick        length_ = 0;   ///< 0 means "the pattern's own length"

    Clip        clip_;
    std::size_t index_  = 0;
    bool        minted_ = false;
};

/// Removes clips.
class RemoveClipsCommand final : public Command {
public:
    explicit RemoveClipsCommand(ClipIds clips) : clips_(std::move(clips)) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.remove"; }
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct RemovedClip {
        std::size_t index = 0;
        Clip        clip;
    };

    ClipIds                  clips_;
    std::vector<RemovedClip> removed_;
};

/// Moves clips in time and across tracks. Mergeable, so a drag is one undo.
///
/// `trackDelta` is a signed row offset rather than a track id: a multi-clip drag
/// moves every clip by the same number of rows, and the clips need not have
/// started on the same track.
class MoveClipsCommand final : public Command {
public:
    MoveClipsCommand(ClipIds clips, Tick tickDelta, int trackDelta)
        : clips_(std::move(clips)), tickDelta_(tickDelta), trackDelta_(trackDelta) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.move"; }
    [[nodiscard]] std::string name() const override { return "Move Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    ClipIds clips_;
    Tick    tickDelta_  = 0;
    int     trackDelta_ = 0;

    /// What was actually applied, which is less than requested when a clip hits
    /// tick zero or the ends of the track list. Undo reverses what happened.
    Tick    appliedTickDelta_  = 0;
    int     appliedTrackDelta_ = 0;

    /// Audio clips are frame-anchored, and tick->frame conversion does not
    /// invert exactly across tempo changes — so their undo restores a
    /// snapshot instead of arithmetic. Recaptured on every execute, which is
    /// what makes redo-after-undo land on the same frames.
    struct MovedAudioClip {
        EntityId               id;
        project::FramePosition previousStart = 0;
    };
    std::vector<MovedAudioClip> movedAudio_;
};

/// Changes clip lengths. Mergeable.
///
/// A pattern clip longer than its pattern repeats it; shorter trims it. Both
/// fall out of project::compileArrangement, so resizing needs no other code.
class ResizeClipsCommand final : public Command {
public:
    ResizeClipsCommand(ClipIds clips, Tick lengthDelta)
        : clips_(std::move(clips)), lengthDelta_(lengthDelta) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.resize"; }
    [[nodiscard]] std::string name() const override { return "Resize Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    ClipIds           clips_;
    Tick              lengthDelta_ = 0;
    std::vector<Tick> previousLengths_;

    /// Aligned with `previousLengths_`; meaningful only for audio clips,
    /// whose lengths live in frames (same snapshot reasoning as moves).
    std::vector<project::FrameCount> previousFrameLengths_;
};

/// Copies clips, offset in time and tracks.
class DuplicateClipsCommand final : public Command {
public:
    DuplicateClipsCommand(ClipIds clips, Tick tickDelta, int trackDelta = 0)
        : clips_(std::move(clips)), tickDelta_(tickDelta), trackDelta_(trackDelta) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.duplicate"; }
    [[nodiscard]] std::string name() const override { return "Duplicate Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// The copies, valid only after the first execute.
    [[nodiscard]] const ClipIds& createdClips() const noexcept { return createdIds_; }

private:
    ClipIds           clips_;
    Tick              tickDelta_  = 0;
    int               trackDelta_ = 0;

    std::vector<Clip> created_;
    ClipIds           createdIds_;
    bool              minted_ = false;
};

/// Splits one clip in two at a timeline tick.
///
/// The left half keeps the clip's identity, fade-in and start; the right half
/// is a new clip whose source offset advances by the left half's length, so
/// both halves keep playing exactly what they played before the cut. Audio
/// clips split on the frame the tick lands on — sample-accurate, and undone
/// from a snapshot because tick→frame does not invert exactly.
class SplitClipCommand final : public Command {
public:
    SplitClipCommand(EntityId clip, Tick splitTick) : clip_(clip), splitTick_(splitTick) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.split"; }
    [[nodiscard]] std::string name() const override { return "Split Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// The created right half, valid after the first execute.
    [[nodiscard]] EntityId rightClipId() const noexcept { return right_.id; }

private:
    EntityId clip_;
    Tick     splitTick_ = 0;

    Clip     previous_;        ///< the unsplit original, for undo
    Clip     right_;           ///< the created half; id minted once, stable across redo
    bool     minted_ = false;
};

class SetClipMutedCommand final : public Command {
public:
    SetClipMutedCommand(ClipIds clips, bool muted) : clips_(std::move(clips)), muted_(muted) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.setMuted"; }
    [[nodiscard]] std::string name() const override { return muted_ ? "Mute Clips" : "Unmute Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    ClipIds           clips_;
    bool              muted_ = false;
    std::vector<bool> previous_;
};

} // namespace incdaw::app
