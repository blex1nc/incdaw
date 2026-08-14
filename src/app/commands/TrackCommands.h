#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
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
    explicit AddTrackCommand(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "track.add"; }
    [[nodiscard]] std::string name() const override { return "Add Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId trackId() const noexcept { return track_.id; }

private:
    std::string name_;
    Track       track_;
    std::size_t index_  = 0;
    bool        minted_ = false;
};

/// Removes a track and every clip on it.
///
/// The clips go with it: a clip whose track is gone is unreachable in the
/// playlist and silent on playback, so leaving it behind would be a leak the
/// user cannot see. Both come back together on undo.
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

    EntityId                 trackId_;
    Track                    track_;
    std::size_t              index_ = 0;
    std::vector<RemovedClip> clips_;
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
