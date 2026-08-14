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

    // A new channel makes a sound immediately. A channel that is silent until
    // an instrument is chosen is a channel the user has to debug before they
    // can hear anything, and every step sequencer in existence gives you a
    // sound the moment you add a row.
    channel.instrument = {plugins::Format::builtin, plugins::builtinSynthUid};

    channels_.push_back(std::move(channel));
    return channels_.back();
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

const Track* Project::findTrack(EntityId id) const noexcept
{
    for (const Track& track : tracks_)
        if (track.id == id)
            return &track;

    return nullptr;
}

const Pattern* Project::findPattern(EntityId id) const noexcept
{
    for (const Pattern& pattern : patterns_)
        if (pattern.id == id)
            return &pattern;

    return nullptr;
}

const MixerNode* Project::findMixerNode(EntityId id) const noexcept
{
    for (const MixerNode& node : mixerNodes_)
        if (node.id == id)
            return &node;

    return nullptr;
}

const Channel* Project::findChannel(EntityId id) const noexcept
{
    for (const Channel& channel : channels_)
        if (channel.id == id)
            return &channel;

    return nullptr;
}

const Clip* Project::findClip(EntityId id) const noexcept
{
    for (const Clip& clip : clips_)
        if (clip.id == id)
            return &clip;

    return nullptr;
}

Pattern* Project::findPatternForEdit(EntityId id) noexcept
{
    return const_cast<Pattern*>(std::as_const(*this).findPattern(id));
}

Channel* Project::findChannelForEdit(EntityId id) noexcept
{
    return const_cast<Channel*>(std::as_const(*this).findChannel(id));
}

EntityId Project::defaultChannel() const noexcept
{
    return channels_.empty() ? EntityId{} : channels_.front().id;
}

bool Project::anyChannelSoloed() const noexcept
{
    for (const Channel& channel : channels_)
        if (channel.soloed)
            return true;

    return false;
}

bool Project::anyTrackSoloed() const noexcept
{
    for (const Track& track : tracks_)
        if (track.soloed)
            return true;

    return false;
}

bool Project::trackIsAudible(EntityId track) const noexcept
{
    const bool anySoloed = anyTrackSoloed();

    // Walks up through folder tracks. A muted folder silences everything inside
    // it, which is the only reason folders are worth having on a busy
    // arrangement. The depth limit is not paranoia: a project file is untrusted
    // input and a parent cycle in it must not hang the compiler.
    EntityId current = track;

    for (int depth = 0; depth < 64 && current.isValid(); ++depth) {
        const Track* entry = findTrack(current);
        if (entry == nullptr)
            return false;

        if (entry->muted)
            return false;

        // Solo applies to the track itself, not to its folders: soloing a
        // folder is expressed by soloing it, and a soloed track inside an
        // unsoloed folder still plays.
        if (current == track && anySoloed && !entry->soloed)
            return false;

        current = entry->parent;
    }

    return true;
}

std::vector<const Clip*> Project::clipsOnTrack(EntityId track) const
{
    std::vector<const Clip*> result;

    for (const Clip& clip : clips_)
        if (clip.track == track)
            result.push_back(&clip);

    std::sort(result.begin(), result.end(),
              [](const Clip* a, const Clip* b) { return a->start < b->start; });

    return result;
}

const PatternChannelSettings* Pattern::settingsFor(EntityId channel) const noexcept
{
    for (const PatternChannelSettings& settings : channelSettings)
        if (settings.channel == channel)
            return &settings;

    return nullptr;
}

Tick Pattern::lengthFor(EntityId channel) const noexcept
{
    const PatternChannelSettings* settings = settingsFor(channel);

    // A per-channel length longer than the pattern is ignored rather than
    // honoured: the pattern's length is what the arrangement places, so a
    // channel that ran past it would be cut off mid-note at every placement.
    if (settings != nullptr && settings->length > 0 && settings->length < length)
        return settings->length;

    return length;
}

double Pattern::swingFor(EntityId channel) const noexcept
{
    const PatternChannelSettings* settings = settingsFor(channel);
    return (settings != nullptr && settings->swing >= 0.0) ? settings->swing : swing;
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
