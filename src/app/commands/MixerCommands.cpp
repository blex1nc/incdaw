#include "app/commands/MixerCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {

// ── AddMixerNodeCommand ───────────────────────────────────────────────────────

bool AddMixerNodeCommand::execute(Project& project)
{
    if (!minted_) {
        const MixerNode& created = project.addMixerNode(type_, name_);
        node_   = created;
        index_  = project.mixerNodes().size() - 1;
        minted_ = true;
        return true;
    }

    project.insertMixerNode(index_, node_);
    return true;
}

void AddMixerNodeCommand::undo(Project& project)
{
    (void)project.removeMixerNode(node_.id);
}

// ── RemoveMixerNodeCommand ────────────────────────────────────────────────────

bool RemoveMixerNodeCommand::execute(Project& project)
{
    if (nodeId_ == project.masterMixerNode())
        return false;

    index_ = project.indexOfMixerNode(nodeId_);
    if (index_ == Project::notFound)
        return false;

    node_ = project.mixerNodes()[index_];

    routing_.clear();

    std::vector<RoutingConnection>& routing = project.routing();

    // Back to front, so each erase leaves the positions recorded for undo valid.
    for (std::size_t index = routing.size(); index > 0; --index) {
        const std::size_t position = index - 1;
        const RoutingConnection& connection = routing[position];

        if (connection.source != nodeId_ && connection.destination != nodeId_)
            continue;

        routing_.push_back({position, connection});
        routing.erase(routing.begin() + static_cast<std::ptrdiff_t>(position));
    }

    // Channels pointed here now point at the master, which is where they would
    // have gone anyway had they never been routed.
    reassignedChannels_.clear();

    for (project::Channel& channel : project.channels()) {
        if (channel.outputMixerNode != nodeId_)
            continue;

        reassignedChannels_.push_back(channel.id);
        channel.outputMixerNode = project.masterMixerNode();
    }

    return project.removeMixerNode(nodeId_);
}

void RemoveMixerNodeCommand::undo(Project& project)
{
    project.insertMixerNode(index_, node_);

    for (auto entry = routing_.rbegin(); entry != routing_.rend(); ++entry)
        project.insertRouting(entry->index, entry->connection);

    for (const EntityId id : reassignedChannels_)
        if (project::Channel* channel = project.findChannel(id))
            channel->outputMixerNode = nodeId_;
}

// ── RenameMixerNodeCommand ────────────────────────────────────────────────────

bool RenameMixerNodeCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr || node->name == name_)
        return false;

    previousName_ = node->name;
    node->name    = name_;
    return true;
}

void RenameMixerNodeCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->name = previousName_;
}

// ── SetMixerVolumeCommand ─────────────────────────────────────────────────────

bool SetMixerVolumeCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr)
        return false;

    // Above unity is deliberately allowed — a mixer that cannot add gain is not
    // a mixer — but not without limit, because a stray drag should not produce
    // +40 dB.
    const double clamped = std::clamp(volume_, 0.0, 4.0);
    if (node->volume == clamped)
        return false;

    previous_    = node->volume;
    volume_      = clamped;
    node->volume = clamped;
    return true;
}

void SetMixerVolumeCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->volume = previous_;
}

bool SetMixerVolumeCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetMixerVolumeCommand*>(&next);
    return other != nullptr && other->nodeId_ == nodeId_;
}

void SetMixerVolumeCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetMixerVolumeCommand*>(&next))
        volume_ = other->volume_;
}

// ── SetMixerPanCommand ────────────────────────────────────────────────────────

bool SetMixerPanCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr)
        return false;

    const double clamped = std::clamp(pan_, -1.0, 1.0);
    if (node->pan == clamped)
        return false;

    previous_ = node->pan;
    pan_      = clamped;
    node->pan = clamped;
    return true;
}

void SetMixerPanCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->pan = previous_;
}

bool SetMixerPanCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetMixerPanCommand*>(&next);
    return other != nullptr && other->nodeId_ == nodeId_;
}

void SetMixerPanCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetMixerPanCommand*>(&next))
        pan_ = other->pan_;
}

// ── Mute, solo, polarity ──────────────────────────────────────────────────────

bool SetMixerMutedCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr || node->muted == muted_)
        return false;

    node->muted = muted_;
    return true;
}

void SetMixerMutedCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->muted = !muted_;
}

bool SetMixerSoloedCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr || node->soloed == soloed_)
        return false;

    node->soloed = soloed_;
    return true;
}

void SetMixerSoloedCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->soloed = !soloed_;
}

bool SetMixerPolarityCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr || node->polarityFlip == inverted_)
        return false;

    node->polarityFlip = inverted_;
    return true;
}

void SetMixerPolarityCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->polarityFlip = !inverted_;
}

// ── SetChannelOutputCommand ───────────────────────────────────────────────────

bool SetChannelOutputCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || project.findMixerNode(mixerNode_) == nullptr)
        return false;

    if (channel->outputMixerNode == mixerNode_)
        return false;

    previous_ = channel->outputMixerNode;
    channel->outputMixerNode = mixerNode_;
    return true;
}

void SetChannelOutputCommand::undo(Project& project)
{
    if (project::Channel* channel = project.findChannel(channelId_))
        channel->outputMixerNode = previous_;
}

// ── ConnectMixerCommand ───────────────────────────────────────────────────────

bool SetMixerStereoSeparationCommand::execute(Project& project)
{
    MixerNode* node = project.findMixerNode(nodeId_);
    if (node == nullptr)
        return false;

    const double clamped = std::clamp(separation_, -1.0, 1.0);
    if (node->stereoSeparation == clamped)
        return false;

    previous_              = node->stereoSeparation;
    separation_            = clamped;
    node->stereoSeparation = clamped;
    return true;
}

void SetMixerStereoSeparationCommand::undo(Project& project)
{
    if (MixerNode* node = project.findMixerNode(nodeId_))
        node->stereoSeparation = previous_;
}

bool SetMixerStereoSeparationCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetMixerStereoSeparationCommand*>(&next);
    return other != nullptr && other->nodeId_ == nodeId_;
}

void SetMixerStereoSeparationCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetMixerStereoSeparationCommand*>(&next))
        separation_ = other->separation_;
}

bool ConnectMixerCommand::execute(Project& project)
{
    if (!minted_) {
        if (project.findMixerNode(source_) == nullptr
            || project.findMixerNode(destination_) == nullptr)
            return false;

        if (source_ == destination_)
            return false;   // a node feeding itself is a cycle, not a route

        RoutingConnection& created = project.connect(source_, destination_);
        created.isSend    = isSend_;
        created.gain      = gain_;
        created.preFader  = preFader_;
        created.sidechain = sidechain_;

        connection_ = created;
        index_      = project.routing().size() - 1;
        minted_     = true;
        return true;
    }

    project.insertRouting(index_, connection_);
    return true;
}

void ConnectMixerCommand::undo(Project& project)
{
    (void)project.removeRouting(connection_.id);
}

// ── DisconnectMixerCommand ────────────────────────────────────────────────────

bool DisconnectMixerCommand::execute(Project& project)
{
    index_ = project.indexOfRouting(connectionId_);
    if (index_ == Project::notFound)
        return false;

    connection_ = project.routing()[index_];
    return project.removeRouting(connectionId_);
}

void DisconnectMixerCommand::undo(Project& project)
{
    project.insertRouting(index_, connection_);
}

// ── SetSendGainCommand ────────────────────────────────────────────────────────

bool SetSendGainCommand::execute(Project& project)
{
    RoutingConnection* connection = project.findRouting(connectionId_);
    if (connection == nullptr)
        return false;

    const double clamped = std::clamp(gain_, 0.0, 4.0);
    if (connection->gain == clamped)
        return false;

    previous_        = connection->gain;
    gain_            = clamped;
    connection->gain = clamped;
    return true;
}

void SetSendGainCommand::undo(Project& project)
{
    if (RoutingConnection* connection = project.findRouting(connectionId_))
        connection->gain = previous_;
}

bool SetSendGainCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetSendGainCommand*>(&next);
    return other != nullptr && other->connectionId_ == connectionId_;
}

void SetSendGainCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetSendGainCommand*>(&next))
        gain_ = other->gain_;
}

} // namespace incdaw::app
