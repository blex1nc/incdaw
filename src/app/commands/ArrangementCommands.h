#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::Clip;
using project::Pattern;
using project::EntityId;
using project::FrameCount;
using project::FramePosition;
using project::Track;

// ── Clips ─────────────────────────────────────────────────────────────────────

// ── Placement ─────────────────────────────────────────────────────────────────

/// Places a pattern on a track.
///
/// The clip references the pattern by id and never copies it — which is the
/// whole point of a pattern: place it twice, edit it once, and both placements
/// change.
class AddPatternClipCommand final : public Command {
public:
    AddPatternClipCommand(EntityId track, EntityId pattern, project::FramePosition start,
                          project::FrameCount length = 0)
        : track_(track), pattern_(pattern), start_(start), length_(length) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.addPatternClip"; }
    [[nodiscard]] std::string name() const override { return "Add Pattern Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdClip() const noexcept { return created_; }

private:
    EntityId               track_;
    EntityId               pattern_;
    project::FramePosition start_  = 0;
    project::FrameCount    length_ = 0;
    EntityId               created_;
};

class DeleteClipCommand final : public Command {
public:
    explicit DeleteClipCommand(EntityId clip) : clip_(clip) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.deleteClip"; }
    [[nodiscard]] std::string name() const override { return "Delete Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    clip_;
    Clip        removed_;
    std::size_t index_ = 0;
};

/// Moves a clip along the timeline, or to another track. Mergeable.
class MoveClipCommand final : public Command {
public:
    MoveClipCommand(EntityId clip, project::FramePosition start, EntityId track = {})
        : clip_(clip), track_(track), start_(start) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.moveClip"; }
    [[nodiscard]] std::string name() const override { return "Move Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId               clip_;
    EntityId               track_;
    project::FramePosition start_ = 0;

    project::FramePosition previousStart_ = 0;
    EntityId               previousTrack_;
};

/// Changes a clip's length by dragging its right edge. Mergeable.
class ResizeClipCommand final : public Command {
public:
    ResizeClipCommand(EntityId clip, FrameCount length) : clip_(clip), length_(length) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.resizeClip"; }
    [[nodiscard]] std::string name() const override { return "Resize Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId   clip_;
    FrameCount length_   = 0;
    FrameCount previous_ = 0;
};

/// Cuts a clip in two at a timeline position.
///
/// The left half keeps the original clip's identity and the right half is a new
/// clip that starts further into the same source. Keeping the left half's id is
/// what lets a selection, an automation reference or an undo entry taken before
/// the split still mean something afterwards.
class SplitClipCommand final : public Command {
public:
    SplitClipCommand(EntityId clip, FramePosition at) : clip_(clip), at_(at) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.splitClip"; }
    [[nodiscard]] std::string name() const override { return "Split Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdClip() const noexcept { return created_; }

private:
    EntityId      clip_;
    FramePosition at_ = 0;

    EntityId      created_;
    FrameCount    originalLength_ = 0;
};

/// Copies a clip and places the copy immediately after it.
class DuplicateClipCommand final : public Command {
public:
    explicit DuplicateClipCommand(EntityId clip) : clip_(clip) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.duplicateClip"; }
    [[nodiscard]] std::string name() const override { return "Duplicate Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdClip() const noexcept { return created_; }

private:
    EntityId clip_;
    EntityId created_;
};

/// Gain and pan for one placement, applied before the mixer. Mergeable.
class SetClipValueCommand final : public Command {
public:
    enum class Property { gain, pan };

    SetClipValueCommand(EntityId clip, Property property, double value)
        : clip_(clip), property_(property), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return property_ == Property::gain ? "playlist.setClipGain" : "playlist.setClipPan";
    }

    [[nodiscard]] std::string name() const override
    {
        return property_ == Property::gain ? "Set Clip Gain" : "Set Clip Pan";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId clip_;
    Property property_ = Property::gain;
    double   value_    = 1.0;
    double   previous_ = 1.0;
};

class SetClipFlagCommand final : public Command {
public:
    enum class Flag { muted, locked, normalize };

    SetClipFlagCommand(EntityId clip, Flag flag, bool value)
        : clip_(clip), flag_(flag), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override;
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId clip_;
    Flag     flag_     = Flag::muted;
    bool     value_    = false;
    bool     previous_ = false;
};

// ── Tracks ────────────────────────────────────────────────────────────────────

class AddTrackCommand final : public Command {
public:
    AddTrackCommand(project::TrackType type, std::string name)
        : type_(type), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.addTrack"; }
    [[nodiscard]] std::string name() const override { return "Add Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdTrack() const noexcept { return created_; }

private:
    project::TrackType type_ = project::TrackType::instrument;
    std::string        name_;
    EntityId           created_;
};

/// Deletes a track and every clip on it.
class DeleteTrackCommand final : public Command {
public:
    explicit DeleteTrackCommand(EntityId track) : track_(track) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.deleteTrack"; }
    [[nodiscard]] std::string name() const override { return "Delete Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId          track_;
    Track             removed_;
    std::size_t       index_ = 0;
    std::vector<Clip> removedClips_;
};

class RenameTrackCommand final : public Command {
public:
    RenameTrackCommand(EntityId track, std::string name)
        : track_(track), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.renameTrack"; }
    [[nodiscard]] std::string name() const override { return "Rename Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    track_;
    std::string name_;
    std::string previous_;
};

class SetTrackFlagCommand final : public Command {
public:
    enum class Flag { muted, soloed };

    SetTrackFlagCommand(EntityId track, Flag flag, bool value)
        : track_(track), flag_(flag), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return flag_ == Flag::muted ? "playlist.setTrackMuted" : "playlist.setTrackSoloed";
    }

    [[nodiscard]] std::string name() const override
    {
        return flag_ == Flag::muted ? "Mute Track" : "Solo Track";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId track_;
    Flag     flag_     = Flag::muted;
    bool     value_    = false;
    bool     previous_ = false;
};

/// Puts a track inside a folder, or takes it out of one.
class SetTrackParentCommand final : public Command {
public:
    SetTrackParentCommand(EntityId track, EntityId parent) : track_(track), parent_(parent) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.setTrackParent"; }
    [[nodiscard]] std::string name() const override { return "Move Track Into Folder"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId track_;
    EntityId parent_;
    EntityId previous_;
};

} // namespace incdaw::app
