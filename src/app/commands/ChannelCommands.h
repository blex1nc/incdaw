#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::Channel;
using project::EntityId;

/// Channel Rack edits.
///
/// Channels are addressed by id, never by row index: the rack is a view of an
/// ordered vector, and an undo entry that meant "row 3" would mean a different
/// channel after any insertion. Ids survive that (docs/ARCHITECTURE.md §5).

/// Adds a channel to the rack.
///
/// The channel is minted once and then replayed on every redo, id included.
/// Minting a fresh id per redo would orphan the pattern content programmed on
/// it and quietly break the redo stack above this entry.
class AddChannelCommand final : public Command {
public:
    explicit AddChannelCommand(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.add"; }
    [[nodiscard]] std::string name() const override { return "Add Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId channelId() const noexcept { return channel_.id; }

private:
    std::string name_;
    Channel     channel_;
    std::size_t index_  = 0;
    bool        minted_ = false;
};

/// Removes a channel and everything programmed on it.
///
/// The channel's content in every pattern goes with it and comes back with it.
/// Leaving orphaned content behind would resurrect on the next channel that
/// happened to reuse the row, and ids are never reused precisely so that
/// cannot happen silently.
class RemoveChannelCommand final : public Command {
public:
    explicit RemoveChannelCommand(EntityId channel) : channelId_(channel) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    /// One pattern's share of the removed channel, and where it sat.
    struct RemovedContent {
        EntityId                       pattern;
        std::size_t                    index = 0;
        project::PatternChannelContent content;
    };

    EntityId                    channelId_;
    Channel                     channel_;
    std::size_t                 index_ = 0;
    std::vector<RemovedContent> content_;
};

class RenameChannelCommand final : public Command {
public:
    RenameChannelCommand(EntityId channel, std::string name)
        : channelId_(channel), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    channelId_;
    std::string name_;
    std::string previousName_;
};

/// Mute and solo.
///
/// Separate commands rather than one parameterised flag setter: the undo menu
/// says what happened, and "Mute Channel" is what happened.
class SetChannelMutedCommand final : public Command {
public:
    SetChannelMutedCommand(EntityId channel, bool muted)
        : channelId_(channel), muted_(muted) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.setMuted"; }
    [[nodiscard]] std::string name() const override { return muted_ ? "Mute Channel" : "Unmute Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId channelId_;
    bool     muted_ = false;
};

class SetChannelSoloedCommand final : public Command {
public:
    SetChannelSoloedCommand(EntityId channel, bool soloed)
        : channelId_(channel), soloed_(soloed) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.setSoloed"; }
    [[nodiscard]] std::string name() const override { return soloed_ ? "Solo Channel" : "Unsolo Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId channelId_;
    bool     soloed_ = false;
};

/// Channel volume, 0..1 linear. Mergeable, so dragging a fader is one undo.
class SetChannelVolumeCommand final : public Command {
public:
    SetChannelVolumeCommand(EntityId channel, double volume)
        : channelId_(channel), volume_(volume) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.setVolume"; }
    [[nodiscard]] std::string name() const override { return "Set Channel Volume"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId channelId_;
    double   volume_         = 1.0;
    double   previousVolume_ = 1.0;
};

/// Moves a channel in the stereo field.
///
/// The model has carried `Channel::pan` since Phase 4 and the graph compiler
/// has applied it since Phase 10 — it was serialized, compiled and audible,
/// and no command could set it. This is that command. Bipolar: -1 hard left,
/// 0 centre, +1 hard right. Consecutive edits merge, so a knob drag is one
/// undo entry.
class SetChannelPanCommand final : public Command {
public:
    SetChannelPanCommand(EntityId channel, double pan)
        : channelId_(channel), pan_(pan) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.setPan"; }
    [[nodiscard]] std::string name() const override { return "Set Channel Pan"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId channelId_;
    double   pan_         = 0.0;
    double   previousPan_ = 0.0;
};

/// One stored instrument parameter (Model.h ChannelInstrumentParameter).
/// The value is plain, in the instrument's own terms; the compiler applies
/// it through the instrument's sink at every build (D-034). Consecutive
/// edits of the same parameter merge, so a slider drag is one undo entry.
class SetInstrumentParameterCommand final : public Command {
public:
    SetInstrumentParameterCommand(EntityId channel, std::uint32_t parameterId, double value)
        : channelId_(channel), parameterId_(parameterId), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return "channel.setInstrumentParameter";
    }
    [[nodiscard]] std::string name() const override { return "Set Instrument Parameter"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId      channelId_;
    std::uint32_t parameterId_ = 0;
    double        value_       = 0.0;

    bool   existedBefore_ = false;
    double previousValue_ = 0.0;
};

/// The pitch this channel's steps are written at.
class SetChannelStepKeyCommand final : public Command {
public:
    SetChannelStepKeyCommand(EntityId channel, int key) : channelId_(channel), key_(key) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.setStepKey"; }
    [[nodiscard]] std::string name() const override { return "Set Step Key"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId channelId_;
    int      key_         = 60;
    int      previousKey_ = 60;
};

} // namespace incdaw::app
