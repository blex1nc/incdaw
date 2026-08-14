#include "project/Model.h"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace incdaw::project {

Project::Project()
{
    metadata_.createdWith = "INCDAW 0.1.0";
    metadata_.lastSavedWith = "INCDAW 0.1.0";

    // Every project has a master from the moment it exists. A project without
    // one would have a window between creation and first edit in which nothing
    // could be routed anywhere.
    master_ = addMixerNode(MixerNodeType::master, "Master").id;
}

Track& Project::addTrack(TrackType type, std::string name)
{
    Track track;
    track.id   = ids_.next();
    track.type = type;
    track.name = std::move(name);
    track.outputMixerNode = master_;

    tracks_.push_back(std::move(track));
    return tracks_.back();
}

Channel& Project::addChannel(std::string name)
{
    Channel channel;
    channel.id   = ids_.next();
    channel.name = std::move(name);
    channel.outputMixerNode = master_;

    channels_.push_back(std::move(channel));
    return channels_.back();
}

Channel& Project::insertChannel(std::size_t index, Channel channel)
{
    ids_.observe(channel.id);

    const std::size_t position = std::min(index, channels_.size());
    return *channels_.insert(channels_.begin() + static_cast<std::ptrdiff_t>(position),
                             std::move(channel));
}

Pattern& Project::insertPattern(std::size_t index, Pattern pattern)
{
    ids_.observe(pattern.id);

    const std::size_t position = std::min(index, patterns_.size());
    return *patterns_.insert(patterns_.begin() + static_cast<std::ptrdiff_t>(position),
                             std::move(pattern));
}

Track& Project::insertTrack(std::size_t index, Track track)
{
    ids_.observe(track.id);

    const std::size_t position = std::min(index, tracks_.size());
    return *tracks_.insert(tracks_.begin() + static_cast<std::ptrdiff_t>(position),
                           std::move(track));
}

Clip& Project::insertClip(std::size_t index, Clip clip)
{
    ids_.observe(clip.id);

    const std::size_t position = std::min(index, clips_.size());
    return *clips_.insert(clips_.begin() + static_cast<std::ptrdiff_t>(position), std::move(clip));
}

MixerNode& Project::insertMixerNode(std::size_t index, MixerNode node)
{
    ids_.observe(node.id);

    const std::size_t position = std::min(index, mixerNodes_.size());
    return *mixerNodes_.insert(mixerNodes_.begin() + static_cast<std::ptrdiff_t>(position),
                               std::move(node));
}

RoutingConnection& Project::insertRouting(std::size_t index, RoutingConnection connection)
{
    ids_.observe(connection.id);

    const std::size_t position = std::min(index, routing_.size());
    return *routing_.insert(routing_.begin() + static_cast<std::ptrdiff_t>(position), connection);
}

bool Project::removeChannel(EntityId id) noexcept
{
    const std::size_t index = indexOfChannel(id);
    if (index == notFound)
        return false;

    channels_.erase(channels_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Project::removePattern(EntityId id) noexcept
{
    const std::size_t index = indexOfPattern(id);
    if (index == notFound)
        return false;

    patterns_.erase(patterns_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Project::removeTrack(EntityId id) noexcept
{
    const std::size_t index = indexOfTrack(id);
    if (index == notFound)
        return false;

    tracks_.erase(tracks_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Project::removeClip(EntityId id) noexcept
{
    const std::size_t index = indexOfClip(id);
    if (index == notFound)
        return false;

    clips_.erase(clips_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

Tick clipStartTicks(const Clip& clip, const engine::TempoMap& tempoMap) noexcept
{
    return clip.type == ClipType::audio ? tempoMap.tickForFrame(clip.start) : clip.startTick;
}

Tick clipLengthTicks(const Clip& clip, const engine::TempoMap& tempoMap) noexcept
{
    if (clip.type != ClipType::audio)
        return clip.lengthTicks;

    // End minus start rather than converting the length alone: with a tempo
    // change inside the clip the two differ, and the end is what must line up
    // on screen with what plays.
    return tempoMap.tickForFrame(clip.start + clip.length) - tempoMap.tickForFrame(clip.start);
}

AudioAsset& Project::insertAudioAsset(std::size_t index, AudioAsset asset)
{
    ids_.observe(asset.id);

    const std::size_t position = std::min(index, audioAssets_.size());
    return *audioAssets_.insert(audioAssets_.begin() + static_cast<std::ptrdiff_t>(position),
                                std::move(asset));
}

bool Project::removeAudioAsset(EntityId id) noexcept
{
    const std::size_t index = indexOfAudioAsset(id);
    if (index == notFound)
        return false;

    audioAssets_.erase(audioAssets_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Project::removeMixerNode(EntityId id) noexcept
{
    // The master is what everything reaches; a project without one cannot be
    // compiled into a graph at all.
    if (id == master_)
        return false;

    const std::size_t index = indexOfMixerNode(id);
    if (index == notFound)
        return false;

    mixerNodes_.erase(mixerNodes_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Project::removeRouting(EntityId id) noexcept
{
    const std::size_t index = indexOfRouting(id);
    if (index == notFound)
        return false;

    routing_.erase(routing_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

std::size_t Project::indexOfChannel(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < channels_.size(); ++index)
        if (channels_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfPattern(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < patterns_.size(); ++index)
        if (patterns_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfTrack(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < tracks_.size(); ++index)
        if (tracks_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfClip(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < clips_.size(); ++index)
        if (clips_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfAudioAsset(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < audioAssets_.size(); ++index)
        if (audioAssets_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfMixerNode(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < mixerNodes_.size(); ++index)
        if (mixerNodes_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfRouting(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < routing_.size(); ++index)
        if (routing_[index].id == id)
            return index;

    return notFound;
}

MixerNode& Project::addMixerNode(MixerNodeType type, std::string name)
{
    MixerNode node;
    node.id   = ids_.next();
    node.type = type;
    node.name = std::move(name);

    mixerNodes_.push_back(std::move(node));
    return mixerNodes_.back();
}

Pattern& Project::addPattern(std::string name)
{
    Pattern pattern;
    pattern.id   = ids_.next();
    pattern.name = std::move(name);

    patterns_.push_back(std::move(pattern));
    return patterns_.back();
}

Clip& Project::addClip(ClipType type, EntityId track, EntityId source)
{
    Clip clip;
    clip.id     = ids_.next();
    clip.type   = type;
    clip.track  = track;
    clip.source = source;

    clips_.push_back(std::move(clip));
    return clips_.back();
}

AutomationLane& Project::addAutomationLane(EntityId target, std::string parameterKey)
{
    AutomationLane lane;
    lane.id           = ids_.next();
    lane.targetEntity = target;
    lane.parameterKey = std::move(parameterKey);

    automation_.push_back(std::move(lane));
    return automation_.back();
}

AudioAsset& Project::addAudioAsset(std::string path)
{
    AudioAsset asset;
    asset.id           = ids_.next();
    asset.absolutePath = std::move(path);

    audioAssets_.push_back(std::move(asset));
    return audioAssets_.back();
}

RoutingConnection& Project::connect(EntityId source, EntityId destination)
{
    RoutingConnection connection;
    connection.id          = ids_.next();
    connection.source      = source;
    connection.destination = destination;

    routing_.push_back(connection);
    return routing_.back();
}

// ── Pattern ───────────────────────────────────────────────────────────────────

const PatternChannelContent* Pattern::content(EntityId channel) const noexcept
{
    for (const PatternChannelContent& entry : channels)
        if (entry.channel == channel)
            return &entry;

    return nullptr;
}

PatternChannelContent* Pattern::content(EntityId channel) noexcept
{
    return const_cast<PatternChannelContent*>(std::as_const(*this).content(channel));
}

PatternChannelContent& Pattern::contentFor(EntityId channel)
{
    if (PatternChannelContent* existing = content(channel))
        return *existing;

    PatternChannelContent entry;
    entry.channel = channel;
    channels.push_back(std::move(entry));
    return channels.back();
}

const std::vector<MidiEvent>* Pattern::events(EntityId channel) const noexcept
{
    const PatternChannelContent* entry = content(channel);
    return entry != nullptr ? &entry->events : nullptr;
}

std::vector<MidiEvent>* Pattern::events(EntityId channel) noexcept
{
    PatternChannelContent* entry = content(channel);
    return entry != nullptr ? &entry->events : nullptr;
}

std::size_t Pattern::totalEventCount() const noexcept
{
    std::size_t count = 0;
    for (const PatternChannelContent& entry : channels)
        count += entry.events.size();

    return count;
}

// ── Lookup ────────────────────────────────────────────────────────────────────

const Track* Project::findTrack(EntityId id) const noexcept
{
    for (const Track& track : tracks_)
        if (track.id == id)
            return &track;

    return nullptr;
}

Track* Project::findTrack(EntityId id) noexcept
{
    return const_cast<Track*>(std::as_const(*this).findTrack(id));
}

const Clip* Project::findClip(EntityId id) const noexcept
{
    for (const Clip& clip : clips_)
        if (clip.id == id)
            return &clip;

    return nullptr;
}

Clip* Project::findClip(EntityId id) noexcept
{
    return const_cast<Clip*>(std::as_const(*this).findClip(id));
}

const Channel* Project::findChannel(EntityId id) const noexcept
{
    for (const Channel& channel : channels_)
        if (channel.id == id)
            return &channel;

    return nullptr;
}

Channel* Project::findChannel(EntityId id) noexcept
{
    return const_cast<Channel*>(std::as_const(*this).findChannel(id));
}

const Pattern* Project::findPattern(EntityId id) const noexcept
{
    for (const Pattern& pattern : patterns_)
        if (pattern.id == id)
            return &pattern;

    return nullptr;
}

Pattern* Project::findPattern(EntityId id) noexcept
{
    return const_cast<Pattern*>(std::as_const(*this).findPattern(id));
}

const MixerNode* Project::findMixerNode(EntityId id) const noexcept
{
    for (const MixerNode& node : mixerNodes_)
        if (node.id == id)
            return &node;

    return nullptr;
}

MixerNode* Project::findMixerNode(EntityId id) noexcept
{
    return const_cast<MixerNode*>(std::as_const(*this).findMixerNode(id));
}

const RoutingConnection* Project::findRouting(EntityId id) const noexcept
{
    for (const RoutingConnection& connection : routing_)
        if (connection.id == id)
            return &connection;

    return nullptr;
}

RoutingConnection* Project::findRouting(EntityId id) noexcept
{
    return const_cast<RoutingConnection*>(std::as_const(*this).findRouting(id));
}

std::vector<EntityId> Project::missingAssets() const
{
    std::vector<EntityId> missing;

    for (const AudioAsset& asset : audioAssets_) {
        if (asset.embedded)
            continue;   // embedded media travels with the package

        const std::string& path = !asset.absolutePath.empty() ? asset.absolutePath : asset.relativePath;

        std::error_code code;
        if (path.empty() || !std::filesystem::exists(path, code))
            missing.push_back(asset.id);
    }

    return missing;
}

bool operator==(const Project& a, const Project& b)
{
    // Metadata timestamps are deliberately excluded: they change on every save
    // and would make the round-trip test compare the clock rather than the model.
    return a.metadata_.title == b.metadata_.title
        && a.metadata_.artist == b.metadata_.artist
        && a.metadata_.comment == b.metadata_.comment
        && a.tempoMap_.tempoEvents() == b.tempoMap_.tempoEvents()
        && a.tempoMap_.timeSignatureEvents() == b.tempoMap_.timeSignatureEvents()
        && a.tracks_ == b.tracks_
        && a.channels_ == b.channels_
        && a.mixerNodes_ == b.mixerNodes_
        && a.patterns_ == b.patterns_
        && a.clips_ == b.clips_
        && a.automation_ == b.automation_
        && a.audioAssets_ == b.audioAssets_
        && a.routing_ == b.routing_
        && a.master_ == b.master_;
}

} // namespace incdaw::project
