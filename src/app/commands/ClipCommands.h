#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace incdaw::app {

using project::Clip;
using project::Track;
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

/// Resizes audio clips by re-stretching their content instead of trimming
/// it — the resize-vs-stretch distinction FL Studio 2026 draws at the clip
/// edge. Length and stretch ratio scale together, so the clip keeps playing
/// the same source span, slower or faster. Mergeable, like resize.
class StretchClipsCommand final : public Command {
public:
    StretchClipsCommand(ClipIds clips, Tick lengthDelta)
        : clips_(std::move(clips)), lengthDelta_(lengthDelta) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.stretch"; }
    [[nodiscard]] std::string name() const override { return "Stretch Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    ClipIds clips_;
    Tick    lengthDelta_ = 0;

    struct Snapshot {
        EntityId                id;
        project::FrameCount     previousLength = 0;
        double                  previousRatio  = 1.0;
    };
    std::vector<Snapshot> previous_;   ///< captured once, before the gesture
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

/// Placement pan, -1 hard left to +1 hard right. Mergeable, so dragging the
/// control is one undo entry.
///
/// The value is a property of the placement, not of the mixer strip the track
/// feeds: two copies of the same audio on the same track can sit in different
/// places in the image.
class SetClipPanCommand final : public Command {
public:
    SetClipPanCommand(ClipIds clips, double pan) : clips_(std::move(clips)), pan_(pan) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.setPan"; }
    [[nodiscard]] std::string name() const override { return "Set Clip Pan"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    ClipIds             clips_;
    double              pan_ = 0.0;
    std::vector<double> previous_;
};

/// Ties clips together so that they move, copy and delete as one.
///
/// The group is project state: it is saved with the song, it survives reopen,
/// and it is not the selection. Selecting one member and dragging moves the
/// lot, which is the whole point — a chorus that was arranged as four clips
/// stays arranged that way.
class GroupClipsCommand final : public Command {
public:
    explicit GroupClipsCommand(ClipIds clips) : clips_(std::move(clips)) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.group"; }
    [[nodiscard]] std::string name() const override { return "Group Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId groupId() const noexcept { return group_; }

private:
    struct Previous {
        EntityId id;
        EntityId group;
    };

    ClipIds               clips_;
    EntityId              group_;    ///< minted once, so redo lands on the same id
    std::vector<Previous> previous_;
};

/// Breaks the group a clip is in — the whole group, not the selection: the
/// gesture undoes a decision, and leaving two of five clips still tied
/// together would be a group nobody asked for.
class UngroupClipsCommand final : public Command {
public:
    explicit UngroupClipsCommand(ClipIds clips) : clips_(std::move(clips)) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.ungroup"; }
    [[nodiscard]] std::string name() const override { return "Ungroup Clips"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct Previous {
        EntityId id;
        EntityId group;
    };

    ClipIds               clips_;
    std::vector<Previous> previous_;
};

/// Recolours clips — a group at a time, like the verbs that move them.
class SetClipColourCommand final : public Command {
public:
    SetClipColourCommand(ClipIds clips, std::uint32_t colour)
        : clips_(std::move(clips)), colour_(colour) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.setColour"; }
    [[nodiscard]] std::string name() const override { return "Set Clip Colour"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct Previous {
        EntityId      id;
        std::uint32_t colour = 0u;
    };

    ClipIds               clips_;
    std::uint32_t         colour_ = 0u;
    std::vector<Previous> previous_;
};

/// Pins a clip in place.
///
/// A locked clip refuses every verb that would change where it is or how long
/// it is — move, resize, stretch, split and remove — while its properties stay
/// editable: a lock protects an arrangement decision, not the mix.
class SetClipLockedCommand final : public Command {
public:
    SetClipLockedCommand(ClipIds clips, bool locked)
        : clips_(std::move(clips)), locked_(locked) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.setLocked"; }
    [[nodiscard]] std::string name() const override
    {
        return locked_ ? "Lock Clips" : "Unlock Clips";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct Previous {
        EntityId id;
        bool     locked = false;
    };

    ClipIds               clips_;
    bool                  locked_ = false;
    std::vector<Previous> previous_;
};

/// Plays an audio clip backwards.
///
/// Only audio clips carry the flag — a pattern plays notes, and reversing a
/// note list is a different operation with a different name. Clips of other
/// types in the selection are left alone rather than silently storing a value
/// nothing will ever read.
class SetClipReversedCommand final : public Command {
public:
    SetClipReversedCommand(ClipIds clips, bool reversed)
        : clips_(std::move(clips)), reversed_(reversed) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.setReversed"; }
    [[nodiscard]] std::string name() const override
    {
        return reversed_ ? "Reverse Clips" : "Unreverse Clips";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct Previous {
        EntityId id;
        bool     reversed = false;
    };

    ClipIds               clips_;
    bool                  reversed_ = false;
    std::vector<Previous> previous_;
};

} // namespace incdaw::app
