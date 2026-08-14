#include "app/commands/ChannelCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {

// ── AddChannelCommand ─────────────────────────────────────────────────────────

bool AddChannelCommand::execute(Project& project)
{
    if (!minted_) {
        const Channel& created = project.addChannel(name_);
        channel_ = created;
        index_   = project.channels().size() - 1;
        minted_  = true;
        return true;
    }

    project.insertChannel(index_, channel_);
    return true;
}

void AddChannelCommand::undo(Project& project)
{
    (void)project.removeChannel(channel_.id);
}

// ── RemoveChannelCommand ──────────────────────────────────────────────────────

bool RemoveChannelCommand::execute(Project& project)
{
    index_ = project.indexOfChannel(channelId_);
    if (index_ == Project::notFound)
        return false;

    channel_ = project.channels()[index_];

    content_.clear();

    for (project::Pattern& pattern : project.patterns()) {
        for (std::size_t index = 0; index < pattern.channels.size(); ++index) {
            if (pattern.channels[index].channel != channelId_)
                continue;

            content_.push_back({pattern.id, index, pattern.channels[index]});
            pattern.channels.erase(pattern.channels.begin() + static_cast<std::ptrdiff_t>(index));
            break;   // Pattern::contentFor keeps at most one block per channel
        }
    }

    return project.removeChannel(channelId_);
}

void RemoveChannelCommand::undo(Project& project)
{
    project.insertChannel(index_, channel_);

    for (const RemovedContent& removed : content_) {
        project::Pattern* pattern = project.findPattern(removed.pattern);
        if (pattern == nullptr)
            continue;

        const std::size_t position = std::min(removed.index, pattern->channels.size());
        pattern->channels.insert(pattern->channels.begin() + static_cast<std::ptrdiff_t>(position),
                                 removed.content);
    }
}

// ── RenameChannelCommand ──────────────────────────────────────────────────────

bool RenameChannelCommand::execute(Project& project)
{
    Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || channel->name == name_)
        return false;

    previousName_ = channel->name;
    channel->name = name_;
    return true;
}

void RenameChannelCommand::undo(Project& project)
{
    if (Channel* channel = project.findChannel(channelId_))
        channel->name = previousName_;
}

// ── SetChannelMutedCommand ────────────────────────────────────────────────────

bool SetChannelMutedCommand::execute(Project& project)
{
    Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || channel->muted == muted_)
        return false;

    channel->muted = muted_;
    return true;
}

void SetChannelMutedCommand::undo(Project& project)
{
    if (Channel* channel = project.findChannel(channelId_))
        channel->muted = !muted_;
}

// ── SetChannelSoloedCommand ───────────────────────────────────────────────────

bool SetChannelSoloedCommand::execute(Project& project)
{
    Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || channel->soloed == soloed_)
        return false;

    channel->soloed = soloed_;
    return true;
}

void SetChannelSoloedCommand::undo(Project& project)
{
    if (Channel* channel = project.findChannel(channelId_))
        channel->soloed = !soloed_;
}

// ── SetChannelVolumeCommand ───────────────────────────────────────────────────

bool SetChannelVolumeCommand::execute(Project& project)
{
    Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr)
        return false;

    const double clamped = std::clamp(volume_, 0.0, 1.0);
    if (channel->volume == clamped)
        return false;

    previousVolume_ = channel->volume;
    volume_         = clamped;
    channel->volume = clamped;
    return true;
}

void SetChannelVolumeCommand::undo(Project& project)
{
    if (Channel* channel = project.findChannel(channelId_))
        channel->volume = previousVolume_;
}

bool SetChannelVolumeCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetChannelVolumeCommand*>(&next);
    return other != nullptr && other->channelId_ == channelId_;
}

void SetChannelVolumeCommand::mergeWith(const Command& next)
{
    // The merged entry keeps the volume the fader started at and adopts where
    // it is now: undoing a drag returns to before the drag, not to its
    // second-to-last position.
    if (const auto* other = dynamic_cast<const SetChannelVolumeCommand*>(&next))
        volume_ = other->volume_;
}

// ── SetChannelStepKeyCommand ──────────────────────────────────────────────────

bool SetChannelStepKeyCommand::execute(Project& project)
{
    Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr)
        return false;

    const int clamped = std::clamp(key_, 0, 127);
    if (channel->stepKey == clamped)
        return false;

    previousKey_     = channel->stepKey;
    key_             = clamped;
    channel->stepKey = clamped;
    return true;
}

void SetChannelStepKeyCommand::undo(Project& project)
{
    if (Channel* channel = project.findChannel(channelId_))
        channel->stepKey = previousKey_;
}

} // namespace incdaw::app
