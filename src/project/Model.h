#pragma once

#include "engine/core/Time.h"
#include "engine/transport/TempoMap.h"
#include "plugins/PluginIdentifier.h"
#include "project/Identity.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace incdaw::project {

using engine::FrameCount;
using engine::FramePosition;
using engine::Tick;

/// CLAUDE.md §24 forbids collapsing these into one generic object. Each entity
/// below is a distinct type with its own identity and lifetime, and they refer
/// to one another by `EntityId` rather than by pointer or index — which is what
/// keeps undo, serialization, and relinking tractable (docs/ARCHITECTURE.md §5).

// ── MIDI ──────────────────────────────────────────────────────────────────────

enum class MidiEventType : std::uint8_t {
    note,
    controlChange,
    pitchBend,
    channelPressure,
    polyPressure,
    programChange,
};

/// A note carries a name and a probability alongside the usual fields.
///
/// Not speculative features: FL Studio 2026 added renamable Piano Roll notes,
/// and per-note probability is long-standing. Both are per-note *properties*,
/// and retrofitting a property onto a fixed struct means touching every piece
/// of code that copies an event. The slot is cheap now and expensive later.
struct MidiEvent {
    MidiEventType type      = MidiEventType::note;
    Tick          tick      = 0;
    Tick          duration  = 0;      ///< notes only
    int           channel   = 0;
    int           key       = 60;     ///< note number, or CC number
    int           value     = 100;    ///< velocity, or CC value
    int           releaseValue = 64;  ///< release velocity
    double        probability  = 1.0; ///< 0..1; notes only
    double        pan          = 0.0; ///< -1..1; per-note
    double        fineTune     = 0.0; ///< semitones
    std::string   label;              ///< user-visible note name

    [[nodiscard]] friend bool operator==(const MidiEvent&, const MidiEvent&) = default;
};

// ── Automation ────────────────────────────────────────────────────────────────

enum class AutomationCurve : std::uint8_t {
    linear,
    hold,       ///< step: value jumps at the point
    smooth,     ///< s-curve
    exponential,
};

struct AutomationPoint {
    Tick            tick    = 0;
    double          value   = 0.0;   ///< normalised 0..1
    AutomationCurve curve   = AutomationCurve::linear;
    double          tension = 0.0;   ///< -1..1, shapes the segment after this point

    [[nodiscard]] friend bool operator==(const AutomationPoint&, const AutomationPoint&) = default;
};

/// One automatable parameter's envelope.
///
/// Deliberately generic: CLAUDE.md §10 forbids building automation separately
/// per plugin or control. A lane names its target by id and parameter key, and
/// knows nothing about what it is automating.
struct AutomationLane {
    EntityId    id;
    EntityId    targetEntity;      ///< mixer node, channel, or plugin instance
    std::string parameterKey;      ///< e.g. "gain", "pan", or a plugin parameter id
    std::vector<AutomationPoint> points;

    [[nodiscard]] friend bool operator==(const AutomationLane&, const AutomationLane&) = default;
};

// ── Patterns ──────────────────────────────────────────────────────────────────

/// Reusable, independently editable musical content.
///
/// A pattern placed several times in the arrangement is the *same* pattern:
/// editing it changes every placement. That is the defining property of the
/// pattern workflow, and it only holds because clips reference a pattern by id
/// rather than owning a copy.
/// One channel's share of a pattern.
///
/// A pattern holds content per channel rather than one flat event list, because
/// that is precisely what a Channel Rack and a step sequencer are: several
/// instruments programmed side by side inside the same pattern. A flat list
/// would force every reader to filter by channel, and would leave nowhere to
/// put a per-channel length.
struct PatternChannelContent {
    EntityId               channel;

    /// Repeat length inside the pattern. 0 follows the pattern's own length; a
    /// shorter value makes this channel loop against the others, which is how
    /// a polymetric pattern is expressed.
    Tick                   loopLength = 0;

    std::vector<MidiEvent> events;

    [[nodiscard]] friend bool operator==(const PatternChannelContent&,
                                         const PatternChannelContent&) = default;
};

struct Pattern {
    EntityId      id;
    std::string   name;
    std::uint32_t colour = 0xFF808080u;
    Tick          length = engine::ticksPerQuarterNote * 4;

    /// Shuffle applied to off-beat subdivisions, 0..1, resolved when the
    /// pattern is compiled. Kept as a property rather than baked into the note
    /// starts so that it stays reversible and the written notes keep meaning
    /// what the user placed.
    double        swing     = 0.0;
    Tick          swingGrid = engine::ticksPerQuarterNote / 4;

    std::vector<PatternChannelContent> channels;
    std::vector<EntityId>              automationLanes;

    [[nodiscard]] const PatternChannelContent* content(EntityId channel) const noexcept;
    [[nodiscard]] PatternChannelContent*       content(EntityId channel) noexcept;

    /// Content for `channel`, created empty if the pattern has none for it yet.
    [[nodiscard]] PatternChannelContent& contentFor(EntityId channel);

    [[nodiscard]] const std::vector<MidiEvent>* events(EntityId channel) const noexcept;
    [[nodiscard]] std::vector<MidiEvent>*       events(EntityId channel) noexcept;

    /// Notes across every channel. Used by things that measure a pattern rather
    /// than edit it.
    [[nodiscard]] std::size_t totalEventCount() const noexcept;

    [[nodiscard]] friend bool operator==(const Pattern&, const Pattern&) = default;
};

// ── Media ─────────────────────────────────────────────────────────────────────

/// An audio file the project uses.
///
/// Carries both a path and a content hash so that a moved file can be found
/// again by content, and a replaced one can be recognised as different. A
/// project that only stored paths would silently open with the wrong audio.
struct AudioAsset {
    EntityId      id;
    std::string   relativePath;   ///< used when the file lives inside the project package
    std::string   absolutePath;   ///< used when it does not
    std::string   contentHash;
    bool          embedded    = false;
    double        sampleRate  = 0.0;
    FrameCount    frameCount  = 0;
    std::size_t   channelCount = 0;

    [[nodiscard]] friend bool operator==(const AudioAsset&, const AudioAsset&) = default;
};

// ── Clips ─────────────────────────────────────────────────────────────────────

enum class ClipType : std::uint8_t { audio, pattern, automation };

/// A placement on the timeline.
///
/// `gain`, `pan` and `normalize` live here rather than on a mixer channel:
/// they are properties of this placement, applied before the mixer, exactly as
/// FL Studio 2026's per-clip gain and normalization work.
struct Clip {
    EntityId      id;
    ClipType      type   = ClipType::pattern;
    EntityId      track;
    EntityId      source;          ///< pattern id, audio asset id, or automation lane id

    /// Musical placement, authoritative for pattern and automation clips.
    ///
    /// A pattern placed at bar 5 belongs at bar 5 whatever the tempo does. Had
    /// the placement been stored in frames, the first tempo change would have
    /// silently desynchronised the whole arrangement.
    Tick          startTick         = 0;
    Tick          lengthTicks       = 0;
    Tick          sourceOffsetTicks = 0;

    /// Sample placement, authoritative for audio clips — they are anchored to
    /// the recording they came from rather than to the beat.
    FramePosition start  = 0;      ///< timeline position
    FrameCount    length = 0;
    FrameCount    sourceOffset = 0;///< frames into the source where playback begins

    double        gain      = 1.0;
    double        pan       = 0.0;
    bool          normalize = false;
    bool          reversed  = false;
    bool          muted     = false;
    bool          locked    = false;

    FrameCount    fadeInFrames  = 0;
    FrameCount    fadeOutFrames = 0;

    double        pitchSemitones = 0.0;
    double        stretchRatio   = 1.0;

    std::string   name;
    std::uint32_t colour = 0xFF6699CCu;

    [[nodiscard]] friend bool operator==(const Clip&, const Clip&) = default;
};

/// A clip's placement in ticks, resolved by type (docs/DECISIONS.md D-013).
///
/// Pattern and automation clips are stored in ticks; audio clips in frames,
/// anchored to the recording they came from. Every consumer that lays clips on
/// a musical grid — the playlist above all — goes through these rather than
/// choosing a field itself, so the two time bases cannot be mixed up.
[[nodiscard]] Tick clipStartTicks(const Clip& clip, const engine::TempoMap& tempoMap) noexcept;
[[nodiscard]] Tick clipLengthTicks(const Clip& clip, const engine::TempoMap& tempoMap) noexcept;

// ── Tracks, channels, mixer ───────────────────────────────────────────────────

enum class TrackType : std::uint8_t { instrument, audio, automation, folder };

struct Track {
    EntityId      id;
    TrackType     type = TrackType::instrument;
    std::string   name;
    std::uint32_t colour = 0xFF505050u;
    EntityId      parent;          ///< folder track, if any
    EntityId      outputMixerNode;
    bool          muted    = false;
    bool          soloed   = false;
    int           height   = 64;   ///< UI row height, persisted with the project

    [[nodiscard]] friend bool operator==(const Track&, const Track&) = default;
};

/// One sample mapping on a sampler channel: which asset, where it sits in
/// pitch, which keys and velocities it answers, and how it loops.
///
/// The numbers deliberately mirror `engine::SamplerZone` — the compiler's job
/// is to swap the asset id for decoded audio and hand the rest through. What
/// the engine type holds as a pointer, the model names by EntityId, because
/// entities refer to one another by id, never by pointer (docs/ARCHITECTURE.md
/// §5): the asset can be relinked or re-embedded without touching the zones.
struct ChannelSamplerZone {
    EntityId   asset;

    int rootKey      = 60;    ///< the key at which the sample plays unshifted
    int keyLow       = 0;
    int keyHigh      = 127;
    int velocityLow  = 1;
    int velocityHigh = 127;

    /// The slice that plays, in source frames. end == 0 means "to the end".
    FrameCount start = 0;
    FrameCount end   = 0;

    /// Sustain loop within the slice, in source frames. loopEnd == 0 means no
    /// loop; the crossfade is the frames blended at the seam.
    FrameCount loopStart     = 0;
    FrameCount loopEnd       = 0;
    FrameCount loopCrossfade = 0;

    bool   reverse = false;
    double gain    = 1.0;

    [[nodiscard]] friend bool operator==(const ChannelSamplerZone&,
                                         const ChannelSamplerZone&) = default;
};

/// One stored instrument parameter value, in the instrument's own plain
/// terms (the id comes from the builtin instrument catalogue, or a hosted
/// instrument's discovery). The model is the source of truth the compiler
/// applies at every build — the same contract mixer volume has — so a value
/// survives rebuilds and save/load without any state blob.
struct ChannelInstrumentParameter {
    std::uint32_t parameterId = 0;
    double        value       = 0.0;

    [[nodiscard]] friend bool operator==(const ChannelInstrumentParameter&,
                                         const ChannelInstrumentParameter&) = default;
};

/// A sound source: instrument, sampler, audio input, or external MIDI device.
struct Channel {
    EntityId      id;
    std::string   name;
    std::uint32_t colour = 0xFF808080u;
    EntityId      outputMixerNode;
    double        volume = 1.0;
    double        pan    = 0.0;
    bool          muted  = false;
    bool          soloed = false;

    /// The pitch a step sequencer step writes on this channel.
    ///
    /// Per channel rather than a global constant because a step grid and a
    /// Piano Roll edit the same notes: a drum channel's steps have to land on
    /// the key its sampler maps that drum to, or the two editors would disagree
    /// about what the user just programmed.
    int           stepKey = 60;

    /// The hosted instrument, if any. Empty uid means "no instrument yet".
    plugins::PluginIdentifier instrument;

    /// Opaque plugin state. INCDAW never interprets it
    /// (docs/PLUGIN_HOST.md §6).
    std::string   instrumentStateFile;

    /// The sampler program, meaningful when `instrument` is the builtin
    /// sampler. Kept on the channel rather than in opaque state because zones
    /// reference audio assets by id, and the relinker has to see that.
    std::vector<ChannelSamplerZone> samplerZones;

    /// Instrument parameter values the user set (panel edits). Applied by the
    /// compiler through the instrument's ParameterSink after construction;
    /// parameters never touched are absent and play at their defaults.
    std::vector<ChannelInstrumentParameter> instrumentParameters;

    [[nodiscard]] friend bool operator==(const Channel&, const Channel&) = default;
};

enum class MixerNodeType : std::uint8_t { track, bus, master };

struct PluginSlot {
    EntityId                 id;
    plugins::PluginIdentifier plugin;
    bool                     bypassed  = false;
    std::string              stateFile;   ///< path within the package's plugins/ directory

    [[nodiscard]] friend bool operator==(const PluginSlot&, const PluginSlot&) = default;
};

struct MixerNode {
    EntityId                id;
    MixerNodeType           type = MixerNodeType::track;
    std::string             name;
    std::uint32_t           colour = 0xFF404040u;
    double                  volume = 1.0;
    double                  pan    = 0.0;
    bool                    muted        = false;
    bool                    soloed       = false;
    bool                    polarityFlip = false;
    std::vector<PluginSlot> inserts;

    [[nodiscard]] friend bool operator==(const MixerNode&, const MixerNode&) = default;
};

/// An edge in the routing graph.
///
/// Routing is a DAG, not a chain: CLAUDE.md §11 forbids a hardcoded linear
/// signal path, and a send is simply a second edge with a gain.
struct RoutingConnection {
    EntityId id;
    EntityId source;
    EntityId destination;
    double   gain      = 1.0;
    bool     isSend    = false;
    bool     preFader  = false;
    bool     sidechain = false;

    [[nodiscard]] friend bool operator==(const RoutingConnection&, const RoutingConnection&) = default;
};

// ── MIDI mappings ─────────────────────────────────────────────────────────────

/// One hardware control bound to one parameter.
///
/// The parameter is named exactly the way an automation lane names its
/// target — a registry key plus a target entity — so a mapped knob and an
/// automation lane are interchangeable views of the same parameter system,
/// and a mapping can drive anything a lane can (CLAUDE.md §22).
struct MidiMapping {
    EntityId    id;
    int         midiChannel = -1;    ///< -1 matches any channel
    int         controller  = 0;     ///< CC number
    std::string parameterKey;        ///< ParameterRegistry key
    EntityId    targetEntity;        ///< mixer node, channel, or insert slot

    /// The mapped output range, normalised. min > max inverts the control.
    double minValue = 0.0;
    double maxValue = 1.0;

    [[nodiscard]] friend bool operator==(const MidiMapping&, const MidiMapping&) = default;
};

// ── Project ───────────────────────────────────────────────────────────────────

struct ProjectMetadata {
    std::string title;
    std::string artist;
    std::string comment;
    std::string createdWith;
    std::string lastSavedWith;
    std::string created;      ///< ISO 8601
    std::string modified;     ///< ISO 8601
};

/// The root. Owns every entity; the engine only ever reads a compiled view.
class Project {
public:
    Project();

    [[nodiscard]] ProjectMetadata&       metadata()       noexcept { return metadata_; }
    [[nodiscard]] const ProjectMetadata& metadata() const noexcept { return metadata_; }

    [[nodiscard]] engine::TempoMap&       tempoMap()       noexcept { return tempoMap_; }
    [[nodiscard]] const engine::TempoMap& tempoMap() const noexcept { return tempoMap_; }

    [[nodiscard]] IdGenerator& ids() noexcept { return ids_; }

    // Creation mints an id and returns a reference to the stored entity.
    Track&             addTrack(TrackType type, std::string name);
    Channel&           addChannel(std::string name);
    MixerNode&         addMixerNode(MixerNodeType type, std::string name);
    Pattern&           addPattern(std::string name);
    Clip&              addClip(ClipType type, EntityId track, EntityId source);
    AutomationLane&    addAutomationLane(EntityId target, std::string parameterKey);
    AudioAsset&        addAudioAsset(std::string path);
    MidiMapping&       addMidiMapping(int controller, std::string parameterKey,
                                      EntityId target);
    RoutingConnection& connect(EntityId source, EntityId destination);

    /// Puts an existing entity back where it was, keeping its id.
    ///
    /// This is what undo needs and `addChannel` cannot provide: re-adding a
    /// removed channel with a fresh id would break every reference to it —
    /// pattern content, routing, and any command still on the redo stack.
    /// `index` is clamped, so a shrunken project restores at the end rather
    /// than out of bounds.
    Channel& insertChannel(std::size_t index, Channel channel);
    Pattern& insertPattern(std::size_t index, Pattern pattern);
    Track&     insertTrack(std::size_t index, Track track);
    Clip&      insertClip(std::size_t index, Clip clip);
    MixerNode& insertMixerNode(std::size_t index, MixerNode node);
    AudioAsset& insertAudioAsset(std::size_t index, AudioAsset asset);
    MidiMapping& insertMidiMapping(std::size_t index, MidiMapping mapping);
    RoutingConnection& insertRouting(std::size_t index, RoutingConnection connection);

    /// Removes an entity by id. False when there is nothing to remove.
    ///
    /// Removing a channel does NOT remove its content from patterns: that
    /// content is captured and restored by the command performing the removal,
    /// which is the only party that can put it back.
    bool removeChannel(EntityId id) noexcept;
    bool removePattern(EntityId id) noexcept;
    bool removeTrack(EntityId id) noexcept;
    bool removeClip(EntityId id) noexcept;
    bool removeMixerNode(EntityId id) noexcept;
    bool removeAudioAsset(EntityId id) noexcept;
    bool removeMidiMapping(EntityId id) noexcept;
    bool removeRouting(EntityId id) noexcept;

    static constexpr std::size_t notFound = static_cast<std::size_t>(-1);

    [[nodiscard]] std::size_t indexOfChannel(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfPattern(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfTrack(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfClip(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfMixerNode(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfAudioAsset(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfMidiMapping(EntityId id) const noexcept;
    [[nodiscard]] std::size_t indexOfRouting(EntityId id) const noexcept;

    [[nodiscard]] std::vector<Track>&             tracks()      noexcept { return tracks_; }
    [[nodiscard]] std::vector<Channel>&           channels()    noexcept { return channels_; }
    [[nodiscard]] std::vector<MixerNode>&         mixerNodes()  noexcept { return mixerNodes_; }
    [[nodiscard]] std::vector<Pattern>&           patterns()    noexcept { return patterns_; }
    [[nodiscard]] std::vector<Clip>&              clips()       noexcept { return clips_; }
    [[nodiscard]] std::vector<AutomationLane>&    automation()  noexcept { return automation_; }
    [[nodiscard]] std::vector<AudioAsset>&        audioAssets() noexcept { return audioAssets_; }
    [[nodiscard]] std::vector<MidiMapping>&       midiMappings() noexcept { return midiMappings_; }
    [[nodiscard]] std::vector<RoutingConnection>& routing()     noexcept { return routing_; }

    [[nodiscard]] const std::vector<Track>&             tracks()      const noexcept { return tracks_; }
    [[nodiscard]] const std::vector<Channel>&           channels()    const noexcept { return channels_; }
    [[nodiscard]] const std::vector<MixerNode>&         mixerNodes()  const noexcept { return mixerNodes_; }
    [[nodiscard]] const std::vector<Pattern>&           patterns()    const noexcept { return patterns_; }
    [[nodiscard]] const std::vector<Clip>&              clips()       const noexcept { return clips_; }
    [[nodiscard]] const std::vector<AutomationLane>&    automation()  const noexcept { return automation_; }
    [[nodiscard]] const std::vector<AudioAsset>&        audioAssets() const noexcept { return audioAssets_; }
    [[nodiscard]] const std::vector<MidiMapping>&       midiMappings() const noexcept { return midiMappings_; }
    [[nodiscard]] const std::vector<RoutingConnection>& routing()     const noexcept { return routing_; }

    [[nodiscard]] EntityId masterMixerNode() const noexcept { return master_; }

    [[nodiscard]] const Track*     findTrack(EntityId id) const noexcept;
    [[nodiscard]] Track*           findTrack(EntityId id) noexcept;
    [[nodiscard]] const Clip*      findClip(EntityId id) const noexcept;
    [[nodiscard]] Clip*            findClip(EntityId id) noexcept;
    [[nodiscard]] const Channel*   findChannel(EntityId id) const noexcept;
    [[nodiscard]] Channel*         findChannel(EntityId id) noexcept;
    [[nodiscard]] const Pattern*   findPattern(EntityId id) const noexcept;
    [[nodiscard]] Pattern*         findPattern(EntityId id) noexcept;
    [[nodiscard]] const MixerNode* findMixerNode(EntityId id) const noexcept;
    [[nodiscard]] MixerNode*       findMixerNode(EntityId id) noexcept;
    [[nodiscard]] const RoutingConnection* findRouting(EntityId id) const noexcept;
    [[nodiscard]] RoutingConnection*       findRouting(EntityId id) noexcept;

    /// Assets whose file cannot be found. A project with missing media still
    /// opens (docs/PROJECT_FORMAT.md §4); this is what the relink dialog lists.
    [[nodiscard]] std::vector<EntityId> missingAssets() const;

    friend bool operator==(const Project&, const Project&);

private:
    ProjectMetadata  metadata_;
    engine::TempoMap tempoMap_;
    IdGenerator      ids_;

    std::vector<Track>             tracks_;
    std::vector<Channel>           channels_;
    std::vector<MixerNode>         mixerNodes_;
    std::vector<Pattern>           patterns_;
    std::vector<Clip>              clips_;
    std::vector<AutomationLane>    automation_;
    std::vector<AudioAsset>        audioAssets_;
    std::vector<MidiMapping>       midiMappings_;
    std::vector<RoutingConnection> routing_;

    EntityId master_;
};

} // namespace incdaw::project
