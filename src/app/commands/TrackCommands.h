#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::Track;

/// Playlist track edits.
///
/// A track is where clips live, not what makes sound: audibility still comes
/// from the channel a clip's pattern is programmed on. Track mute and solo
/// therefore decide which *placements* are compiled, which is why they are
/// resolved in project::compileArrangement rather than in the graph.

class AddTrackCommand final : public Command {
public:
    /// The type defaults so that every existing caller keeps its meaning; a
    /// folder is created through the same verb rather than a second command,
    /// because it is the same entity in the same list.
    explicit AddTrackCommand(std::string name,
                             project::TrackType type = project::TrackType::instrument)
        : name_(std::move(name)), type_(type) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.add"; }
    [[nodiscard]] std::string name() const override
    {
        return type_ == project::TrackType::folder ? "Add Folder" : "Add Track";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId trackId() const noexcept { return track_.id; }

private:
    std::string        name_;
    project::TrackType type_ = project::TrackType::instrument;
    Track              track_;
    std::size_t        index_  = 0;
    bool               minted_ = false;
};

/// Removes a track and every clip on it.
///
/// The clips go with it: a clip whose track is gone is unreachable in the
/// playlist and silent on playback, so leaving it behind would be a leak the
/// user cannot see. Both come back together on undo.
///
/// Removing a folder does not remove what was inside it. Its children move up
/// to its own parent, so closing a group keeps the tracks and loses only the
/// grouping — deleting a folder row and silently deleting eight tracks of work
/// are the same gesture, and only one of them is what anyone means.
class RemoveTrackCommand final : public Command {
public:
    explicit RemoveTrackCommand(EntityId track) : trackId_(track) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct RemovedClip {
        std::size_t   index = 0;
        project::Clip clip;
    };

    struct Reparented {
        EntityId track;
        EntityId previousParent;
    };

    EntityId                 trackId_;
    Track                    track_;
    std::size_t              index_ = 0;
    std::vector<RemovedClip> clips_;
    std::vector<Reparented>  reparented_;
};

/// Moves a track into a folder, out of one, or between two.
///
/// The track (and, for a folder, everything under it) also moves in the track
/// list to sit directly after its new parent: the playlist draws that list in
/// order, and a folder whose children are scattered through it is a folder in
/// name only.
class SetTrackParentCommand final : public Command {
public:
    /// An invalid `parent` moves the track back out to the top level.
    SetTrackParentCommand(EntityId track, EntityId parent)
        : trackId_(track), parent_(parent) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.setParent"; }
    [[nodiscard]] std::string name() const override
    {
        return parent_.isValid() ? "Move Track Into Folder" : "Move Track Out Of Folder";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId trackId_;
    EntityId parent_;

    EntityId           previousParent_;
    std::vector<Track> previousOrder_;   ///< the whole list, restored wholesale
};

/// Opens and closes a folder in the playlist.
///
/// Presentation only: the children keep playing either way. It is undoable and
/// it is saved, because it is a state the user arranged deliberately.
class SetTrackCollapsedCommand final : public Command {
public:
    SetTrackCollapsedCommand(EntityId track, bool collapsed)
        : trackId_(track), collapsed_(collapsed) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.setCollapsed"; }
    [[nodiscard]] std::string name() const override
    {
        return collapsed_ ? "Collapse Folder" : "Expand Folder";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId trackId_;
    bool     collapsed_ = false;
};

class SetTrackColourCommand final : public Command {
public:
    SetTrackColourCommand(EntityId track, std::uint32_t colour)
        : trackId_(track), colour_(colour) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.setColour"; }
    [[nodiscard]] std::string name() const override { return "Set Track Colour"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId      trackId_;
    std::uint32_t colour_         = 0u;
    std::uint32_t previousColour_ = 0u;
};

class RenameTrackCommand final : public Command {
public:
    RenameTrackCommand(EntityId track, std::string name)
        : trackId_(track), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    trackId_;
    std::string name_;
    std::string previousName_;
};

class SetTrackMutedCommand final : public Command {
public:
    SetTrackMutedCommand(EntityId track, bool muted) : trackId_(track), muted_(muted) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.setMuted"; }
    [[nodiscard]] std::string name() const override { return muted_ ? "Mute Track" : "Unmute Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId trackId_;
    bool     muted_ = false;
};

class SetTrackSoloedCommand final : public Command {
public:
    SetTrackSoloedCommand(EntityId track, bool soloed) : trackId_(track), soloed_(soloed) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.setSoloed"; }
    [[nodiscard]] std::string name() const override { return soloed_ ? "Solo Track" : "Unsolo Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId trackId_;
    bool     soloed_ = false;
};

/// Row height in the playlist, persisted with the project. Mergeable, so a drag
/// on the row edge is one undo.
class SetTrackHeightCommand final : public Command {
public:
    SetTrackHeightCommand(EntityId track, int height) : trackId_(track), height_(height) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.setHeight"; }
    [[nodiscard]] std::string name() const override { return "Resize Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId trackId_;
    int      height_         = 64;
    int      previousHeight_ = 64;
};

} // namespace incdaw::app
