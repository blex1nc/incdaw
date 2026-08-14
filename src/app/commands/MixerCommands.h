#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::MixerNode;
using project::RoutingConnection;

/// Mixer and routing edits.
///
/// Routing is a graph, not a chain (CLAUDE.md §11): a send is simply a second
/// edge with a gain, and a bus is a mixer node other things are routed into.
/// These commands therefore edit nodes and edges, never a fixed signal path.

class AddMixerNodeCommand final : public Command {
public:
    AddMixerNodeCommand(project::MixerNodeType type, std::string name)
        : type_(type), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.add"; }
    [[nodiscard]] std::string name() const override { return "Add Mixer Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId mixerNodeId() const noexcept { return node_.id; }

private:
    project::MixerNodeType type_;
    std::string            name_;
    MixerNode              node_;
    std::size_t            index_  = 0;
    bool                   minted_ = false;
};

/// Removes a mixer node, its routing, and any channel assignment to it.
///
/// Leaving edges behind that name a node which no longer exists would compile
/// to a graph quietly missing connections, which is worse than a visible
/// change. The master cannot be removed: everything reaches it.
class RemoveMixerNodeCommand final : public Command {
public:
    explicit RemoveMixerNodeCommand(EntityId node) : nodeId_(node) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Mixer Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct RemovedRouting {
        std::size_t       index = 0;
        RoutingConnection connection;
    };

    EntityId                    nodeId_;
    MixerNode                   node_;
    std::size_t                 index_ = 0;
    std::vector<RemovedRouting> routing_;

    /// Channels that pointed at this node, and are re-pointed at the master.
    std::vector<EntityId>       reassignedChannels_;
};

class RenameMixerNodeCommand final : public Command {
public:
    RenameMixerNodeCommand(EntityId node, std::string name)
        : nodeId_(node), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Mixer Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    nodeId_;
    std::string name_;
    std::string previousName_;
};

/// Fader. Mergeable, so a move is one undo.
class SetMixerVolumeCommand final : public Command {
public:
    SetMixerVolumeCommand(EntityId node, double volume) : nodeId_(node), volume_(volume) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setVolume"; }
    [[nodiscard]] std::string name() const override { return "Set Mixer Volume"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId nodeId_;
    double   volume_   = 1.0;
    double   previous_ = 1.0;
};

/// Pan, -1..1. Mergeable.
class SetMixerPanCommand final : public Command {
public:
    SetMixerPanCommand(EntityId node, double pan) : nodeId_(node), pan_(pan) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setPan"; }
    [[nodiscard]] std::string name() const override { return "Set Mixer Pan"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId nodeId_;
    double   pan_      = 0.0;
    double   previous_ = 0.0;
};

class SetMixerMutedCommand final : public Command {
public:
    SetMixerMutedCommand(EntityId node, bool muted) : nodeId_(node), muted_(muted) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setMuted"; }
    [[nodiscard]] std::string name() const override { return muted_ ? "Mute Mixer Track" : "Unmute Mixer Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId nodeId_;
    bool     muted_ = false;
};

class SetMixerSoloedCommand final : public Command {
public:
    SetMixerSoloedCommand(EntityId node, bool soloed) : nodeId_(node), soloed_(soloed) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setSoloed"; }
    [[nodiscard]] std::string name() const override { return soloed_ ? "Solo Mixer Track" : "Unsolo Mixer Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId nodeId_;
    bool     soloed_ = false;
};

class SetMixerPolarityCommand final : public Command {
public:
    SetMixerPolarityCommand(EntityId node, bool inverted) : nodeId_(node), inverted_(inverted) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setPolarity"; }
    [[nodiscard]] std::string name() const override { return "Invert Polarity"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId nodeId_;
    bool     inverted_ = false;
};

/// Points a channel at a mixer node.
class SetChannelOutputCommand final : public Command {
public:
    SetChannelOutputCommand(EntityId channel, EntityId mixerNode)
        : channelId_(channel), mixerNode_(mixerNode) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setChannelOutput"; }
    [[nodiscard]] std::string name() const override { return "Route Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId channelId_;
    EntityId mixerNode_;
    EntityId previous_;
};

/// Adds an edge between two mixer nodes: a route, or a send with its own gain.
class ConnectMixerCommand final : public Command {
public:
    ConnectMixerCommand(EntityId source, EntityId destination, bool isSend = false,
                        double gain = 1.0, bool preFader = false)
        : source_(source), destination_(destination), isSend_(isSend), gain_(gain),
          preFader_(preFader) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.connect"; }
    [[nodiscard]] std::string name() const override { return isSend_ ? "Add Send" : "Route Mixer Track"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId connectionId() const noexcept { return connection_.id; }

private:
    EntityId          source_;
    EntityId          destination_;
    bool              isSend_   = false;
    double            gain_     = 1.0;
    bool              preFader_ = false;

    RoutingConnection connection_;
    std::size_t       index_  = 0;
    bool              minted_ = false;
};

class DisconnectMixerCommand final : public Command {
public:
    explicit DisconnectMixerCommand(EntityId connection) : connectionId_(connection) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.disconnect"; }
    [[nodiscard]] std::string name() const override { return "Remove Routing"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId          connectionId_;
    RoutingConnection connection_;
    std::size_t       index_ = 0;
};

/// Send level. Mergeable.
class SetSendGainCommand final : public Command {
public:
    SetSendGainCommand(EntityId connection, double gain) : connectionId_(connection), gain_(gain) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.setSendGain"; }
    [[nodiscard]] std::string name() const override { return "Set Send Level"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId connectionId_;
    double   gain_     = 1.0;
    double   previous_ = 1.0;
};

} // namespace incdaw::app
