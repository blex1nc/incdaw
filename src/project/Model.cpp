#include "project/Model.h"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace incdaw::project {
namespace {

/// Default colours for new material, rotated by creation order.
///
/// A project in which every channel, pattern and track is the same grey forces
/// the user to colour their own work before the window can be read at a glance;
/// both of the workflows INCDAW takes as reference assign a colour on creation
/// instead. These are INCDAW's own hues — not sampled from anyone's product —
/// and they are ordinary project data the user can overwrite at any time.
constexpr std::uint32_t defaultColours[] = {
    0xFF3AA9FFu,   // blue
    0xFF46D97Fu,   // green
    0xFFFFC24Au,   // amber
    0xFFFF6B5Eu,   // coral
    0xFFC07BFFu,   // violet
    0xFF34D6C8u,   // teal
    0xFFFF9FD1u,   // pink
    0xFF9BE45Cu,   // lime
};

[[nodiscard]] std::uint32_t colourForIndex(std::size_t index) noexcept
{
    return defaultColours[index % (sizeof(defaultColours) / sizeof(defaultColours[0]))];
}

} // namespace


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
    track.colour = colourForIndex(tracks_.size());
    track.outputMixerNode = master_;

    tracks_.push_back(std::move(track));
    return tracks_.back();
}

Channel& Project::addChannel(std::string name)
{
    Channel channel;
    channel.id   = ids_.next();
    channel.name = std::move(name);
    channel.colour = colourForIndex(channels_.size());
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

MidiMapping& Project::addMidiMapping(int controller, std::string parameterKey, EntityId target)
{
    MidiMapping mapping;
    mapping.id           = ids_.next();
    mapping.controller   = controller;
    mapping.parameterKey = std::move(parameterKey);
    mapping.targetEntity = target;

    midiMappings_.push_back(std::move(mapping));
    return midiMappings_.back();
}

MidiMapping& Project::insertMidiMapping(std::size_t index, MidiMapping mapping)
{
    ids_.observe(mapping.id);

    const std::size_t position = std::min(index, midiMappings_.size());
    return *midiMappings_.insert(midiMappings_.begin() + static_cast<std::ptrdiff_t>(position),
                                 std::move(mapping));
}

bool Project::removeMidiMapping(EntityId id) noexcept
{
    const std::size_t index = indexOfMidiMapping(id);
    if (index == notFound)
        return false;

    midiMappings_.erase(midiMappings_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

TimelineMarker& Project::addMarker(Tick tick, std::string name)
{
    TimelineMarker marker;
    marker.id   = ids_.next();
    marker.tick = tick;
    marker.name = std::move(name);

    markers_.push_back(std::move(marker));
    return markers_.back();
}

TimelineMarker& Project::insertMarker(std::size_t index, TimelineMarker marker)
{
    ids_.observe(marker.id);

    const std::size_t position = std::min(index, markers_.size());
    return *markers_.insert(markers_.begin() + static_cast<std::ptrdiff_t>(position),
                            std::move(marker));
}

bool Project::removeMarker(EntityId id) noexcept
{
    const std::size_t index = indexOfMarker(id);
    if (index == notFound)
        return false;

    markers_.erase(markers_.begin() + static_cast<std::ptrdiff_t>(index));
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

std::size_t Project::indexOfMidiMapping(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < midiMappings_.size(); ++index)
        if (midiMappings_[index].id == id)
            return index;

    return notFound;
}

std::size_t Project::indexOfMarker(EntityId id) const noexcept
{
    for (std::size_t index = 0; index < markers_.size(); ++index)
        if (markers_[index].id == id)
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

    // The master keeps a neutral plate; inserts and buses take the rotation, so
    // a strip can be found by colour the same way its channel can.
    node.colour = type == MixerNodeType::master ? 0xFF8894A6u
                                                : colourForIndex(mixerNodes_.size());

    mixerNodes_.push_back(std::move(node));
    return mixerNodes_.back();
}

Pattern& Project::addPattern(std::string name)
{
    Pattern pattern;
    pattern.id   = ids_.next();
    pattern.name = std::move(name);
    pattern.colour = colourForIndex(patterns_.size());

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

const TimelineMarker* Project::findMarker(EntityId id) const noexcept
{
    for (const TimelineMarker& marker : markers_)
        if (marker.id == id)
            return &marker;

    return nullptr;
}

TimelineMarker* Project::findMarker(EntityId id) noexcept
{
    return const_cast<TimelineMarker*>(std::as_const(*this).findMarker(id));
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
        && a.midiMappings_ == b.midiMappings_
        && a.markers_ == b.markers_
        && a.routing_ == b.routing_
        && a.master_ == b.master_;
}

// ── Folder tracks ─────────────────────────────────────────────────────────────

namespace {

/// Walks up the parent chain, calling `predicate` on each track including the
/// one it starts at, and stops at the first true.
///
/// The step budget is the track count: a chain longer than that has revisited
/// a track, which means a cycle, and the walk gives up rather than spinning.
/// The commands refuse to build one — this is the belt to that pair of braces,
/// because a project file can be edited by hand.
const Track* findIn(const std::vector<Track>& tracks, EntityId id) noexcept
{
    if (!id.isValid())
        return nullptr;

    const auto found = std::find_if(tracks.begin(), tracks.end(),
                                    [id](const Track& track) { return track.id == id; });

    return found == tracks.end() ? nullptr : &*found;
}

template <typename Predicate>
bool anyAncestor(const std::vector<Track>& tracks, const Track& track, Predicate predicate)
{
    const Track* current = &track;

    for (std::size_t step = 0; step <= tracks.size(); ++step) {
        if (predicate(*current))
            return true;

        current = findIn(tracks, current->parent);
        if (current == nullptr)
            return false;
    }

    return false;
}

template <typename Predicate>
bool anyAncestor(const Project& project, const Track& track, Predicate predicate)
{
    return anyAncestor(project.tracks(), track, predicate);
}

} // namespace

bool trackEffectivelyMuted(const Project& project, const Track& track) noexcept
{
    return anyAncestor(project, track, [](const Track& node) { return node.muted; });
}

bool trackEffectivelySoloed(const Project& project, const Track& track) noexcept
{
    return anyAncestor(project, track, [](const Track& node) { return node.soloed; });
}

bool trackHidden(const std::vector<Track>& tracks, const Track& track) noexcept
{
    const Track* parent = findIn(tracks, track.parent);
    if (parent == nullptr)
        return false;

    // The track's own collapsed flag hides its children, not itself, so the
    // walk starts one level up.
    return anyAncestor(tracks, *parent, [](const Track& node) { return node.collapsed; });
}

bool trackHidden(const Project& project, const Track& track) noexcept
{
    return trackHidden(project.tracks(), track);
}

std::size_t trackDepth(const std::vector<Track>& tracks, const Track& track) noexcept
{
    std::size_t depth   = 0;
    const Track* parent = findIn(tracks, track.parent);

    // Bounded by the list, like every other walk here: a cycle stops at the
    // count rather than running away with the drawing code.
    while (parent != nullptr && depth <= tracks.size()) {
        ++depth;
        parent = findIn(tracks, parent->parent);
    }

    return depth;
}

bool trackWouldCycle(const Project& project, EntityId track, EntityId parent) noexcept
{
    if (!track.isValid() || !parent.isValid())
        return false;
    if (track == parent)
        return true;

    const Track* candidate = project.findTrack(parent);
    if (candidate == nullptr)
        return false;

    return anyAncestor(project, *candidate,
                       [track](const Track& node) { return node.id == track; });
}

const Clip* crossfadePartner(const Project& project, const Clip& clip, bool incoming) noexcept
{
    if (clip.type != ClipType::audio)
        return nullptr;
    if (incoming ? !clip.crossfadeIn : !clip.crossfadeOut)
        return nullptr;

    const FramePosition start = clip.start;
    const FramePosition end   = clip.start + static_cast<FramePosition>(clip.length);

    const Clip* best = nullptr;

    for (const Clip& other : project.clips()) {
        if (other.id == clip.id || other.type != ClipType::audio)
            continue;
        if (other.track != clip.track || other.lane != clip.lane)
            continue;

        const FramePosition otherStart = other.start;
        const FramePosition otherEnd   = other.start
                                       + static_cast<FramePosition>(other.length);

        // The incoming edge pairs with a clip that started earlier and is
        // still sounding; the outgoing edge with one that starts inside this
        // clip and carries on.
        const bool matches = incoming
                                 ? (other.crossfadeOut && otherStart < start && otherEnd > start)
                                 : (other.crossfadeIn && otherStart > start && otherStart < end);
        if (!matches)
            continue;

        // Nearest wins, so a stack of three keeps its two separate pairs.
        if (best == nullptr
            || (incoming ? other.start > best->start : other.start < best->start))
            best = &other;
    }

    return best;
}

ClipFades clipFades(const Project& project, const Clip& clip) noexcept
{
    ClipFades fades{clip.fadeInFrames, clip.fadeOutFrames};

    if (const Clip* partner = crossfadePartner(project, clip, true)) {
        const FramePosition partnerEnd = partner->start
                                       + static_cast<FramePosition>(partner->length);
        const FramePosition overlapEnd =
            std::min(partnerEnd, clip.start + static_cast<FramePosition>(clip.length));

        fades.in = static_cast<FrameCount>(std::max<FramePosition>(0, overlapEnd - clip.start));
    }

    if (const Clip* partner = crossfadePartner(project, clip, false)) {
        const FramePosition end = clip.start + static_cast<FramePosition>(clip.length);
        const FramePosition overlapStart = std::max(partner->start, clip.start);

        fades.out = static_cast<FrameCount>(std::max<FramePosition>(0, end - overlapStart));
    }

    return fades;
}

int trackLaneCount(const Project& project, EntityId track) noexcept
{
    int highest = 0;

    for (const Clip& clip : project.clips())
        if (clip.track == track)
            highest = std::max(highest, clip.lane);

    return highest + 1;
}

std::vector<EntityId> tracksUnder(const Project& project, EntityId folder)
{
    std::vector<EntityId> under;
    if (!folder.isValid())
        return under;

    for (const Track& track : project.tracks()) {
        if (track.id == folder)
            continue;

        if (anyAncestor(project, track, [folder](const Track& node) { return node.id == folder; }))
            under.push_back(track.id);
    }

    return under;
}

} // namespace incdaw::project
