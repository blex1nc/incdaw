#include "app/commands/PatternCommands.h"

#include "project/InstrumentFactory.h"

#include <algorithm>

namespace incdaw::app {
namespace {

template <typename Entity>
[[nodiscard]] std::size_t indexOf(const std::vector<Entity>& entities, EntityId id) noexcept
{
    for (std::size_t index = 0; index < entities.size(); ++index)
        if (entities[index].id == id)
            return index;

    return entities.size();
}

/// Restores an entity at the position it was removed from.
///
/// Order matters even where the audio does not care: the channel rack and the
/// pattern list show these in order, and an undo that reappears at the bottom
/// of the list rather than where it was is an undo the user has to re-read.
template <typename Entity>
void restoreAt(std::vector<Entity>& entities, std::size_t index, Entity entity)
{
    const std::size_t position = std::min(index, entities.size());
    entities.insert(entities.begin() + static_cast<std::ptrdiff_t>(position), std::move(entity));
}

/// The tick a step falls on, and the notes that already occupy it.
[[nodiscard]] Tick tickForStep(const Pattern& pattern, int step) noexcept
{
    const Tick division = pattern.stepDivision > 0 ? pattern.stepDivision : 1;
    return static_cast<Tick>(step) * division;
}

[[nodiscard]] bool noteOwnedBy(const project::MidiEvent& event, EntityId channel,
                               EntityId defaultChannel) noexcept
{
    const EntityId owner = event.channelId.isValid() ? event.channelId : defaultChannel;
    return owner == channel;
}

} // namespace

// ── AddPatternCommand ─────────────────────────────────────────────────────────

bool AddPatternCommand::execute(Project& project)
{
    if (!created_.isValid()) {
        Pattern& pattern = project.addPattern(name_);
        created_ = pattern.id;
        return true;
    }

    // Redo. The id is reused so that clips placed before the undo still point
    // at this pattern.
    Pattern pattern;
    pattern.id   = created_;
    pattern.name = name_;
    project.ids().observe(created_);
    project.patterns().push_back(std::move(pattern));
    return true;
}

void AddPatternCommand::undo(Project& project)
{
    auto& patterns = project.patterns();
    const std::size_t index = indexOf(patterns, created_);

    if (index < patterns.size())
        patterns.erase(patterns.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── DuplicatePatternCommand ───────────────────────────────────────────────────

bool DuplicatePatternCommand::execute(Project& project)
{
    const Pattern* source = project.findPattern(source_);
    if (source == nullptr)
        return false;

    Pattern copy = *source;
    copy.name += " copy";

    if (!created_.isValid())
        created_ = project.ids().next();

    copy.id = created_;
    project.ids().observe(created_);
    project.patterns().push_back(std::move(copy));
    return true;
}

void DuplicatePatternCommand::undo(Project& project)
{
    auto& patterns = project.patterns();
    const std::size_t index = indexOf(patterns, created_);

    if (index < patterns.size())
        patterns.erase(patterns.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── DeletePatternCommand ──────────────────────────────────────────────────────

bool DeletePatternCommand::execute(Project& project)
{
    auto& patterns = project.patterns();
    index_ = indexOf(patterns, pattern_);

    if (index_ >= patterns.size())
        return false;

    removed_ = patterns[index_];
    patterns.erase(patterns.begin() + static_cast<std::ptrdiff_t>(index_));

    removedClips_.clear();
    auto& clips = project.clips();

    for (const Clip& clip : clips)
        if (clip.type == project::ClipType::pattern && clip.source == pattern_)
            removedClips_.push_back(clip);

    clips.erase(std::remove_if(clips.begin(), clips.end(),
                               [this](const Clip& clip) {
                                   return clip.type == project::ClipType::pattern
                                       && clip.source == pattern_;
                               }),
                clips.end());

    return true;
}

void DeletePatternCommand::undo(Project& project)
{
    restoreAt(project.patterns(), index_, removed_);

    for (const Clip& clip : removedClips_)
        project.clips().push_back(clip);
}

// ── RenamePatternCommand ──────────────────────────────────────────────────────

bool RenamePatternCommand::execute(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr || pattern->name == name_)
        return false;

    previous_     = pattern->name;
    pattern->name = name_;
    return true;
}

void RenamePatternCommand::undo(Project& project)
{
    if (Pattern* pattern = project.findPatternForEdit(pattern_))
        pattern->name = previous_;
}

// ── SetPatternLengthCommand ───────────────────────────────────────────────────

namespace {

/// The per-channel settings entry for a channel, creating it if asked.
project::PatternChannelSettings* settingsFor(Pattern& pattern, EntityId channel, bool create,
                                             bool& created)
{
    created = false;

    for (project::PatternChannelSettings& settings : pattern.channelSettings)
        if (settings.channel == channel)
            return &settings;

    if (!create)
        return nullptr;

    project::PatternChannelSettings settings;
    settings.channel = channel;
    pattern.channelSettings.push_back(settings);
    created = true;
    return &pattern.channelSettings.back();
}

void dropSettings(Pattern& pattern, EntityId channel)
{
    auto& settings = pattern.channelSettings;
    settings.erase(std::remove_if(settings.begin(), settings.end(),
                                  [channel](const project::PatternChannelSettings& entry) {
                                      return entry.channel == channel;
                                  }),
                   settings.end());
}

} // namespace

bool SetPatternLengthCommand::execute(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr || length_ <= 0)
        return false;

    if (!channel_.isValid()) {
        if (pattern->length == length_)
            return false;

        previous_       = pattern->length;
        pattern->length = length_;
        return true;
    }

    project::PatternChannelSettings* settings = settingsFor(*pattern, channel_, true, addedSettings_);
    if (settings == nullptr || settings->length == length_)
        return false;

    previous_        = settings->length;
    settings->length = length_;
    return true;
}

void SetPatternLengthCommand::undo(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr)
        return;

    if (!channel_.isValid()) {
        pattern->length = previous_;
        return;
    }

    if (addedSettings_) {
        dropSettings(*pattern, channel_);
        return;
    }

    bool created = false;
    if (project::PatternChannelSettings* settings = settingsFor(*pattern, channel_, false, created))
        settings->length = previous_;
}

bool SetPatternLengthCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetPatternLengthCommand*>(&next);
    return other != nullptr && other->pattern_ == pattern_ && other->channel_ == channel_;
}

void SetPatternLengthCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetPatternLengthCommand*>(&next))
        length_ = other->length_;
}

// ── SetPatternSwingCommand ────────────────────────────────────────────────────

bool SetPatternSwingCommand::execute(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr)
        return false;

    const double clamped = std::clamp(swing_, 0.0, 1.0);

    if (!channel_.isValid()) {
        if (pattern->swing == clamped)
            return false;

        previous_      = pattern->swing;
        pattern->swing = clamped;
        return true;
    }

    project::PatternChannelSettings* settings = settingsFor(*pattern, channel_, true, addedSettings_);
    if (settings == nullptr || settings->swing == clamped)
        return false;

    previous_       = settings->swing;
    settings->swing = clamped;
    return true;
}

void SetPatternSwingCommand::undo(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr)
        return;

    if (!channel_.isValid()) {
        pattern->swing = previous_;
        return;
    }

    if (addedSettings_) {
        dropSettings(*pattern, channel_);
        return;
    }

    bool created = false;
    if (project::PatternChannelSettings* settings = settingsFor(*pattern, channel_, false, created))
        settings->swing = previous_;
}

bool SetPatternSwingCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetPatternSwingCommand*>(&next);
    return other != nullptr && other->pattern_ == pattern_ && other->channel_ == channel_;
}

void SetPatternSwingCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetPatternSwingCommand*>(&next))
        swing_ = other->swing_;
}

// ── AddChannelCommand ─────────────────────────────────────────────────────────

bool AddChannelCommand::execute(Project& project)
{
    if (!created_.isValid()) {
        Channel& channel = project.addChannel(name_);
        created_ = channel.id;
        return true;
    }

    Channel channel;
    channel.id              = created_;
    channel.name            = name_;
    channel.outputMixerNode = project.masterMixerNode();
    channel.instrument      = project::defaultInstrumentIdentifier();
    project.ids().observe(created_);
    project.channels().push_back(std::move(channel));
    return true;
}

void AddChannelCommand::undo(Project& project)
{
    auto& channels = project.channels();
    const std::size_t index = indexOf(channels, created_);

    if (index < channels.size())
        channels.erase(channels.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── DeleteChannelCommand ──────────────────────────────────────────────────────

bool DeleteChannelCommand::execute(Project& project)
{
    auto& channels = project.channels();
    index_ = indexOf(channels, channel_);

    if (index_ >= channels.size())
        return false;

    // Captured before the channel goes, because "which notes belonged to it"
    // depends on which channel is first — deleting the first channel changes
    // what an untagged note means.
    const EntityId defaultChannel = project.defaultChannel();

    removed_ = channels[index_];
    removedNotes_.clear();

    for (Pattern& pattern : project.patterns()) {
        RemovedNotes removal;
        removal.pattern = pattern.id;

        for (std::size_t index = 0; index < pattern.events.size(); ++index) {
            if (noteOwnedBy(pattern.events[index], channel_, defaultChannel)) {
                removal.indices.push_back(index);
                removal.events.push_back(pattern.events[index]);
            }
        }

        if (removal.indices.empty())
            continue;

        for (std::size_t offset = removal.indices.size(); offset > 0; --offset)
            pattern.events.erase(pattern.events.begin()
                                 + static_cast<std::ptrdiff_t>(removal.indices[offset - 1]));

        removedNotes_.push_back(std::move(removal));
    }

    channels.erase(channels.begin() + static_cast<std::ptrdiff_t>(index_));
    return true;
}

void DeleteChannelCommand::undo(Project& project)
{
    restoreAt(project.channels(), index_, removed_);

    for (const RemovedNotes& removal : removedNotes_) {
        Pattern* pattern = project.findPatternForEdit(removal.pattern);
        if (pattern == nullptr)
            continue;

        // Reinserted lowest index first, so each note lands back where it was.
        for (std::size_t offset = 0; offset < removal.indices.size(); ++offset) {
            const std::size_t index = std::min(removal.indices[offset], pattern->events.size());
            pattern->events.insert(pattern->events.begin() + static_cast<std::ptrdiff_t>(index),
                                   removal.events[offset]);
        }
    }
}

// ── RenameChannelCommand ──────────────────────────────────────────────────────

bool RenameChannelCommand::execute(Project& project)
{
    Channel* channel = project.findChannelForEdit(channel_);
    if (channel == nullptr || channel->name == name_)
        return false;

    previous_     = channel->name;
    channel->name = name_;
    return true;
}

void RenameChannelCommand::undo(Project& project)
{
    if (Channel* channel = project.findChannelForEdit(channel_))
        channel->name = previous_;
}

// ── SetChannelValueCommand ────────────────────────────────────────────────────

bool SetChannelValueCommand::execute(Project& project)
{
    Channel* channel = project.findChannelForEdit(channel_);
    if (channel == nullptr)
        return false;

    const double value = property_ == Property::volume ? std::clamp(value_, 0.0, 2.0)
                                                       : std::clamp(value_, -1.0, 1.0);

    double& target = property_ == Property::volume ? channel->volume : channel->pan;

    if (target == value)
        return false;

    previous_ = target;
    target    = value;
    return true;
}

void SetChannelValueCommand::undo(Project& project)
{
    Channel* channel = project.findChannelForEdit(channel_);
    if (channel == nullptr)
        return;

    (property_ == Property::volume ? channel->volume : channel->pan) = previous_;
}

bool SetChannelValueCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetChannelValueCommand*>(&next);
    return other != nullptr && other->channel_ == channel_ && other->property_ == property_;
}

void SetChannelValueCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetChannelValueCommand*>(&next))
        value_ = other->value_;
}

// ── SetChannelFlagCommand ─────────────────────────────────────────────────────

bool SetChannelFlagCommand::execute(Project& project)
{
    Channel* channel = project.findChannelForEdit(channel_);
    if (channel == nullptr)
        return false;

    bool& target = flag_ == Flag::muted ? channel->muted : channel->soloed;

    if (target == value_)
        return false;

    previous_ = target;
    target    = value_;
    return true;
}

void SetChannelFlagCommand::undo(Project& project)
{
    Channel* channel = project.findChannelForEdit(channel_);
    if (channel == nullptr)
        return;

    (flag_ == Flag::muted ? channel->muted : channel->soloed) = previous_;
}

// ── ToggleStepCommand ─────────────────────────────────────────────────────────

bool ToggleStepCommand::execute(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr || step_ < 0)
        return false;

    const Tick     tick           = tickForStep(*pattern, step_);
    const EntityId defaultChannel = project.defaultChannel();

    for (std::size_t index = 0; index < pattern->events.size(); ++index) {
        const project::MidiEvent& event = pattern->events[index];

        if (event.type != project::MidiEventType::note || event.tick != tick)
            continue;

        if (!noteOwnedBy(event, channel_, defaultChannel))
            continue;

        removed_      = event;
        removedIndex_ = index;
        turnedOn_     = false;
        pattern->events.erase(pattern->events.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    project::MidiEvent note;
    note.type      = project::MidiEventType::note;
    note.tick      = tick;
    note.duration  = pattern->stepDivision > 0 ? pattern->stepDivision : 1;
    note.key       = key_;
    note.value     = velocity_;
    note.channelId = channel_;

    removedIndex_ = pattern->events.size();
    turnedOn_     = true;
    pattern->events.push_back(note);
    return true;
}

void ToggleStepCommand::undo(Project& project)
{
    Pattern* pattern = project.findPatternForEdit(pattern_);
    if (pattern == nullptr)
        return;

    if (turnedOn_) {
        if (removedIndex_ < pattern->events.size())
            pattern->events.erase(pattern->events.begin()
                                  + static_cast<std::ptrdiff_t>(removedIndex_));
        return;
    }

    const std::size_t index = std::min(removedIndex_, pattern->events.size());
    pattern->events.insert(pattern->events.begin() + static_cast<std::ptrdiff_t>(index), removed_);
}

// ── AddPatternClipCommand ─────────────────────────────────────────────────────

bool AddPatternClipCommand::execute(Project& project)
{
    const Pattern* pattern = project.findPattern(pattern_);
    if (pattern == nullptr)
        return false;

    const project::FrameCount length =
        length_ > 0 ? length_ : project.tempoMap().frameForTick(pattern->length);

    if (!created_.isValid()) {
        Clip& clip = project.addClip(project::ClipType::pattern, track_, pattern_);
        clip.start  = start_;
        clip.length = length;
        clip.name   = pattern->name;
        clip.colour = pattern->colour;
        created_    = clip.id;
        return true;
    }

    Clip clip;
    clip.id     = created_;
    clip.type   = project::ClipType::pattern;
    clip.track  = track_;
    clip.source = pattern_;
    clip.start  = start_;
    clip.length = length;
    clip.name   = pattern->name;
    clip.colour = pattern->colour;
    project.ids().observe(created_);
    project.clips().push_back(std::move(clip));
    return true;
}

void AddPatternClipCommand::undo(Project& project)
{
    auto& clips = project.clips();
    const std::size_t index = indexOf(clips, created_);

    if (index < clips.size())
        clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── DeleteClipCommand ─────────────────────────────────────────────────────────

bool DeleteClipCommand::execute(Project& project)
{
    auto& clips = project.clips();
    index_ = indexOf(clips, clip_);

    if (index_ >= clips.size())
        return false;

    removed_ = clips[index_];
    clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(index_));
    return true;
}

void DeleteClipCommand::undo(Project& project)
{
    restoreAt(project.clips(), index_, removed_);
}

// ── MoveClipCommand ───────────────────────────────────────────────────────────

bool MoveClipCommand::execute(Project& project)
{
    Clip* target = nullptr;

    for (Clip& clip : project.clips())
        if (clip.id == clip_)
            target = &clip;

    if (target == nullptr || target->locked)
        return false;

    const project::FramePosition start = std::max<project::FramePosition>(0, start_);
    const EntityId track = track_.isValid() ? track_ : target->track;

    if (target->start == start && target->track == track)
        return false;

    previousStart_ = target->start;
    previousTrack_ = target->track;

    target->start = start;
    target->track = track;
    return true;
}

void MoveClipCommand::undo(Project& project)
{
    for (Clip& clip : project.clips()) {
        if (clip.id != clip_)
            continue;

        clip.start = previousStart_;
        clip.track = previousTrack_;
        return;
    }
}

bool MoveClipCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const MoveClipCommand*>(&next);
    return other != nullptr && other->clip_ == clip_;
}

void MoveClipCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const MoveClipCommand*>(&next)) {
        start_ = other->start_;
        track_ = other->track_.isValid() ? other->track_ : track_;
    }
}

} // namespace incdaw::app
