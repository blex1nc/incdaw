#include "project/ProjectGraphCompiler.h"

#include "engine/audio/AudioClipNode.h"
#include "engine/audio/InputMonitorNode.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/instrument/SimpleSynth.h"
#include "engine/automation/AutomationNode.h"
#include "project/PatternCompiler.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <unordered_map>

namespace incdaw::project {
namespace {

/// True when `channel` should be heard, given the project's solo state.
///
/// Solo is exclusive across the project rather than per-channel: the moment any
/// channel is soloed, every channel that is not becomes silent. That is what
/// solo means everywhere, and getting it wrong is immediately obvious.
bool isAudible(const Channel& channel, bool anySoloed) noexcept
{
    if (channel.muted)
        return false;

    return !anySoloed || channel.soloed;
}

} // namespace

InstrumentFactory defaultInstrumentFactory()
{
    return [](const Channel&) -> std::unique_ptr<engine::Instrument> {
        return std::make_unique<engine::SimpleSynth>();
    };
}

engine::InstrumentNode* CompiledProjectGraph::instrumentFor(EntityId channel) const noexcept
{
    for (std::size_t index = 0; index < channels.size(); ++index)
        if (channels[index] == channel)
            return instruments[index];

    return nullptr;
}

engine::dsp::MixerStripNode* CompiledProjectGraph::stripFor(EntityId mixerNode) const noexcept
{
    for (std::size_t index = 0; index < mixerNodes.size(); ++index)
        if (mixerNodes[index] == mixerNode)
            return strips[index];

    return nullptr;
}

engine::dsp::MixerStripNode* CompiledProjectGraph::channelStripFor(EntityId channel) const noexcept
{
    for (std::size_t index = 0; index < channels.size(); ++index)
        if (channels[index] == channel)
            return channelStrips[index];

    return nullptr;
}

CompiledProjectGraph compileProjectGraph(const Project& project, const engine::TempoMap& tempoMap,
                                         const GraphCompileOptions& options)
{
    CompiledProjectGraph compiled;

    const InstrumentFactory factory = options.instrumentFactory ? options.instrumentFactory
                                                                : defaultInstrumentFactory();

    const Pattern* patternToPlay = nullptr;
    if (options.source == PlaybackSource::pattern) {
        patternToPlay = options.pattern.isValid() ? project.findPattern(options.pattern)
                      : (project.patterns().empty() ? nullptr : &project.patterns().front());
    }

    const bool anySoloed = std::any_of(project.channels().begin(), project.channels().end(),
                                       [](const Channel& channel) { return channel.soloed; });

    const bool anyMixerSoloed = std::any_of(project.mixerNodes().begin(), project.mixerNodes().end(),
                                            [&project](const MixerNode& node) {
                                                return node.soloed && node.id != project.masterMixerNode();
                                            });

    engine::GraphBuilder builder;

    // ── Mixer nodes ──────────────────────────────────────────────────────────
    // Every mixer node becomes a strip, whether or not anything reaches it: a
    // strip the user can see in the mixer but that vanishes from the graph
    // would meter nothing and accept no fader move.
    std::unordered_map<EntityId, engine::NodeIndex> stripIndices;
    std::vector<EntityId>                           mixerIds;
    std::vector<engine::dsp::MixerStripNode*>       stripNodes;

    /// Where signal ENTERS a mixer node: the head of its insert chain, or the
    /// strip itself when it has no inserts. Separate from `stripIndices`,
    /// which is where signal LEAVES. Collapsing the two would route a send
    /// into the destination's fader and past its plugins.
    std::unordered_map<EntityId, engine::NodeIndex> stripInputIndices;

    engine::NodeIndex master = engine::invalidNode;

    for (const MixerNode& node : project.mixerNodes()) {
        auto strip  = std::make_unique<engine::dsp::MixerStripNode>();
        auto* handle = strip.get();

        const bool isMaster = node.id == project.masterMixerNode();

        // Solo is exclusive across the mixer, and the master is exempt: soloing
        // a track must not silence the output everything reaches.
        const bool silenced = node.muted || (anyMixerSoloed && !node.soloed && !isMaster);

        handle->setGain(static_cast<engine::Sample>(node.volume)
                        * (isMaster ? options.masterGain : engine::Sample{1}));
        handle->setPan(node.pan);
        handle->setMuted(silenced);
        handle->setPolarityInverted(node.polarityFlip);

        const auto index = builder.addNode(std::move(strip));
        stripIndices.emplace(node.id, index);

        // ── This strip's insert chain ────────────────────────────────────
        // Inserts run BEFORE the fader: everything arriving at the strip is
        // summed by the first insert, handed down the chain, and only then
        // scaled by volume, panned, inverted and muted. A fader move must
        // not change what a compressor hears.
        std::vector<engine::NodeIndex> chain;

        for (const PluginSlot& slot : node.inserts) {
            if (slot.bypassed)
                continue;   // bypass is absence, not a node multiplying by one

            if (!options.insertFactory) {
                compiled.warnings.push_back("insert \"" + slot.plugin.toString() + "\" on \""
                                            + node.name + "\" is silent: this build has no "
                                            + "plugin host");
                continue;
            }

            std::string insertError;
            auto insertNode = options.insertFactory(slot, insertError);

            if (insertNode == nullptr) {
                // Pass-through, not silence and not a failed compile: a
                // missing plugin must never cost the user the rest of the mix.
                compiled.warnings.push_back("insert \"" + slot.plugin.toString() + "\" on \""
                                            + node.name + "\" is bypassed: " + insertError);
                continue;
            }

            chain.push_back(builder.addNode(std::move(insertNode)));
        }

        engine::NodeIndex chainInput = index;

        if (!chain.empty()) {
            for (std::size_t position = 0; position + 1 < chain.size(); ++position)
                builder.connect(chain[position], chain[position + 1]);

            builder.connect(chain.back(), index);
            chainInput = chain.front();
        }

        stripInputIndices.emplace(node.id, chainInput);

        mixerIds.push_back(node.id);
        stripNodes.push_back(handle);

        if (isMaster)
            master = index;
    }

    if (master == engine::invalidNode) {
        // A project without a master still has to make a graph: the master
        // exists from the moment a project does, but a corrupted file might not
        // have one, and silence with an error beats a crash.
        auto strip = std::make_unique<engine::dsp::MixerStripNode>();
        strip->setGain(options.masterGain);

        master = builder.addNode(std::move(strip));
        stripNodes.push_back(nullptr);
        mixerIds.push_back(EntityId{});
        stripNodes.back() = nullptr;
    }

    // Where anything that reaches the master directly enters it: the head of
    // the master's insert chain. A signal that skipped it would be the one
    // thing in the project a master-bus limiter could not catch.
    engine::NodeIndex masterInput = master;

    if (const auto found = stripInputIndices.find(project.masterMixerNode());
        found != stripInputIndices.end())
        masterInput = found->second;

    // ── Routing between mixer nodes ─────────────────────────────────────────
    for (const RoutingConnection& connection : project.routing()) {
        // Out of the source's fader, into the head of the destination's
        // insert chain.
        const auto source      = stripIndices.find(connection.source);
        const auto destination = stripInputIndices.find(connection.destination);

        if (source == stripIndices.end() || destination == stripInputIndices.end())
            continue;   // an edge naming something that is not a mixer node

        if (connection.sidechain)
            continue;   // sidechain has no meaning until a plugin can receive it

        if (!connection.isSend) {
            builder.connect(source->second, destination->second);
            continue;
        }

        // A send is a second edge with its own gain, so it needs a node to
        // carry that gain.
        const auto send = builder.addNode(std::make_unique<engine::dsp::GainNode>(
            static_cast<engine::Sample>(connection.gain)));

        builder.connect(source->second, send);
        builder.connect(send, destination->second);
    }

    // ── Channels ────────────────────────────────────────────────────────────
    std::vector<engine::InstrumentNode*>      instrumentNodes;
    std::vector<EntityId>                     channelIds;
    std::vector<engine::dsp::MixerStripNode*> channelStripNodes;

    for (const Channel& channel : project.channels()) {
        if (!isAudible(channel, anySoloed))
            continue;

        std::unique_ptr<engine::Instrument> instrument = factory(channel);
        if (instrument == nullptr)
            continue;   // a channel with no working instrument is silent, not an error

        auto node    = std::make_unique<engine::InstrumentNode>(std::move(instrument), tempoMap);
        auto* handle = node.get();

        if (patternToPlay != nullptr)
            compilePatternInto(handle->sequence(), *patternToPlay, channel.id, options.randomSeed);
        else if (options.source == PlaybackSource::arrangement)
            compileArrangementInto(handle->sequence(), project, channel.id, options.randomSeed);

        const auto source = builder.addNode(std::move(node));

        // The channel's own strip: volume and pan belong to the channel, not to
        // the mixer track it feeds, which may be shared by several channels.
        auto channelStrip = std::make_unique<engine::dsp::MixerStripNode>();
        auto* channelHandle = channelStrip.get();
        channelHandle->setGain(static_cast<engine::Sample>(channel.volume));
        channelHandle->setPan(channel.pan);

        const auto strip = builder.addNode(std::move(channelStrip));

        const auto destination = stripInputIndices.find(channel.outputMixerNode);

        builder.connect(source, strip);
        builder.connect(strip,
                        destination != stripInputIndices.end() ? destination->second : masterInput);

        channelIds.push_back(channel.id);
        instrumentNodes.push_back(handle);
        channelStripNodes.push_back(channelHandle);
    }

    // ── Audio clips ─────────────────────────────────────────────────────────
    // One AudioClipNode per audio track, the way one InstrumentNode carries a
    // channel. Assets are decoded here, once each, and shared between clips;
    // the shared_ptrs die with the graph, off the audio thread, like
    // everything else the reaper handles.
    if (options.source == PlaybackSource::arrangement) {
        std::unordered_map<EntityId, std::shared_ptr<engine::AudioFileData>> loadedAssets;

        const auto assetPath = [&](EntityId id) -> const std::string* {
            for (const AudioAsset& candidate : project.audioAssets())
                if (candidate.id == id)
                    return !candidate.absolutePath.empty() ? &candidate.absolutePath
                                                           : &candidate.relativePath;
            return nullptr;
        };

        const auto loadAsset =
            [&](EntityId id) -> std::shared_ptr<engine::AudioFileData> {
            if (const auto found = loadedAssets.find(id); found != loadedAssets.end())
                return found->second;

            std::shared_ptr<engine::AudioFileData> loaded;

            if (const std::string* path = assetPath(id)) {
                auto data = std::make_shared<engine::AudioFileData>();

                if (const auto read = engine::WavFile::read(*path, *data); !read) {
                    compiled.warnings.push_back("audio asset unreadable: " + *path
                                                + " (" + read.error + ")");
                } else if (data->sampleRate != options.sampleRate) {
                    // Playing it anyway would be the wrong pitch and the wrong
                    // length. Silence with a reason beats subtly wrong audio;
                    // import-time sample-rate conversion is recorded as
                    // outstanding work.
                    compiled.warnings.push_back(
                        "audio asset at " + std::to_string(data->sampleRate)
                        + " Hz in a " + std::to_string(options.sampleRate)
                        + " Hz session, left silent: " + *path);
                } else {
                    loaded = std::move(data);
                }
            }

            loadedAssets.emplace(id, loaded);
            return loaded;
        };

        /// True when this asset should stream rather than preload. Decided
        /// from the header alone (probe), never by decoding.
        const auto shouldStream = [&](EntityId id) -> bool {
            if (options.diskStreamer == nullptr)
                return false;

            const std::string* path = assetPath(id);
            if (path == nullptr)
                return false;

            engine::AudioFileData header;
            return bool(engine::WavFile::probe(*path, header))
                && header.frameCount > options.streamingThresholdFrames;
        };

        /// One stream PER CLIP, not per asset: two clips of the same file at
        /// different positions would fight over a shared window and starve
        /// each other into permanent underrun. A window is ~1 MB; a fight is
        /// audible.
        const auto openStream =
            [&](EntityId id, engine::FrameCount prefillAt) -> std::shared_ptr<engine::AudioStream> {
            const std::string* path = assetPath(id);
            if (path == nullptr)
                return nullptr;

            auto stream = std::make_shared<engine::AudioStream>();

            if (const auto opened = stream->open(*path); !opened) {
                compiled.warnings.push_back("audio asset unreadable: " + *path
                                            + " (" + opened.error + ")");
                return nullptr;
            }

            if (stream->sampleRate() != options.sampleRate) {
                compiled.warnings.push_back(
                    "audio asset at " + std::to_string(stream->sampleRate())
                    + " Hz in a " + std::to_string(options.sampleRate)
                    + " Hz session, left silent: " + *path);
                return nullptr;
            }

            // Warm the window here, off the audio thread, so a graph swap
            // never begins with an underrun on its first block.
            stream->prefill(prefillAt);
            options.diskStreamer->add(stream);
            return stream;
        };

        const bool anyTrackSoloed = std::any_of(project.tracks().begin(), project.tracks().end(),
                                                [](const Track& track) { return track.soloed; });

        for (const Track& track : project.tracks()) {
            if (track.type != TrackType::audio)
                continue;
            if (track.muted || (anyTrackSoloed && !track.soloed))
                continue;

            auto node = std::make_unique<engine::AudioClipNode>();

            for (const Clip& clip : project.clips()) {
                if (clip.type != ClipType::audio || clip.track != track.id || clip.muted)
                    continue;

                engine::AudioClipNode::PlacedClip placed;

                if (shouldStream(clip.source)) {
                    placed.stream = openStream(clip.source, clip.sourceOffset);
                    if (placed.stream == nullptr)
                        continue;
                } else {
                    placed.audio = loadAsset(clip.source);
                    if (placed.audio == nullptr)
                        continue;
                }

                placed.start         = clip.start;
                placed.length        = clip.length;
                placed.sourceOffset  = clip.sourceOffset;
                placed.gain          = static_cast<engine::Sample>(clip.gain);
                placed.fadeInFrames  = clip.fadeInFrames;
                placed.fadeOutFrames = clip.fadeOutFrames;

                // Clip normalize folds into the placement gain here, at
                // compile time — the node stays one multiply per sample. The
                // peak is the CLIP'S content (the source range it plays),
                // not the whole file's.
                if (clip.normalize) {
                    if (placed.audio != nullptr) {
                        const engine::FrameCount from =
                            std::min(clip.sourceOffset, placed.audio->frameCount);
                        const engine::FrameCount to =
                            std::min(clip.sourceOffset + clip.length, placed.audio->frameCount);

                        engine::Sample peak = 0.0f;
                        for (const auto& channel : placed.audio->channels)
                            for (engine::FrameCount frame = from; frame < to; ++frame)
                                peak = std::max(peak,
                                                std::abs(channel[static_cast<std::size_t>(frame)]));

                        if (peak > 0.0f)
                            placed.gain /= peak;
                    } else {
                        // A streamed clip's exact peak would cost a full file
                        // pass on every rebuild. Deferred honestly rather
                        // than approximated silently.
                        compiled.warnings.push_back(
                            "normalize on a streamed clip is not yet supported; played at "
                            "clip gain: " + clip.name);
                    }
                }

                node->addClip(std::move(placed));
            }

            if (node->clipCount() == 0)
                continue;   // an empty node would render nothing at some cost

            const auto source      = builder.addNode(std::move(node));
            const auto destination = stripInputIndices.find(track.outputMixerNode);

            builder.connect(source, destination != stripInputIndices.end() ? destination->second
                                                                           : masterInput);
        }
    }

    // ── Input monitoring ────────────────────────────────────────────────────
    // Session state, not project data: the engine's monitor ring drains into
    // the master when monitoring is on. Rebuilding the graph is how the
    // toggle takes effect, exactly like every other topology change.
    if (options.monitorRing != nullptr && options.monitorChannelCount > 0) {
        const auto monitor = builder.addNode(std::make_unique<engine::InputMonitorNode>(
            options.monitorRing, options.monitorChannelCount));
        builder.connect(monitor, masterInput);
    }

    // ── Automation ──────────────────────────────────────────────────────────
    // One node evaluates every lane, once per block, and writes through the
    // same smoothed setters the mixer's own fader uses. Nothing here names a
    // parameter: resolution goes through the registry, which is the whole of
    // the "no parameter-specific code" requirement.
    const ParameterRegistry builtins = ParameterRegistry::withBuiltins();
    const ParameterRegistry& registry = options.parameters != nullptr ? *options.parameters
                                                                      : builtins;

    auto automation = std::make_unique<engine::AutomationNode>(tempoMap);

    /// Where a lane writes: shifted by `offsetTicks`, only inside the window.
    struct LanePlacement {
        Tick offsetTicks = 0;
        Tick windowStart = std::numeric_limits<Tick>::min();
        Tick windowEnd   = std::numeric_limits<Tick>::max();
    };

    const auto bindLane = [&](const AutomationLane& lane, const LanePlacement& placement) {
        if (lane.points.empty())
            return;

        const ParameterRegistry::Entry* parameter = registry.find(lane.parameterKey);
        if (parameter == nullptr || !parameter->apply)
            return;   // a lane naming an unknown parameter is data, not an error

        // The lane's target resolves to whichever strip renders it: a mixer
        // node's own strip, or the channel strip of a channel.
        engine::dsp::MixerStripNode* strip = nullptr;

        if (const auto found = stripIndices.find(lane.targetEntity); found != stripIndices.end()) {
            for (std::size_t index = 0; index < mixerIds.size(); ++index)
                if (mixerIds[index] == lane.targetEntity)
                    strip = stripNodes[index];
        } else {
            for (std::size_t index = 0; index < channelIds.size(); ++index)
                if (channelIds[index] == lane.targetEntity)
                    strip = channelStripNodes[index];
        }

        if (strip == nullptr)
            return;   // silent channel, or a target that no longer exists

        engine::AutomationNode::Binding binding;
        binding.windowStart = placement.windowStart;
        binding.windowEnd   = placement.windowEnd;

        std::vector<engine::AutomationPoint> points;
        points.reserve(lane.points.size());

        for (const AutomationPoint& point : lane.points) {
            engine::AutomationPoint translated;
            translated.tick    = point.tick + placement.offsetTicks;
            translated.value   = static_cast<float>(point.value);
            translated.tension = static_cast<float>(point.tension);

            switch (point.curve) {
                case AutomationCurve::hold:        translated.shape = engine::AutomationShape::hold; break;
                case AutomationCurve::smooth:      translated.shape = engine::AutomationShape::smooth; break;
                case AutomationCurve::exponential: translated.shape = engine::AutomationShape::exponential; break;
                case AutomationCurve::linear:      translated.shape = engine::AutomationShape::linear; break;
            }

            points.push_back(translated);
        }

        binding.sequence.setPoints(std::move(points));

        // The applier is copied into the binding: the registry may be the
        // stack-local builtins above, and a reference into it would dangle the
        // moment this function returns.
        binding.apply = [strip, apply = parameter->apply](float value) { apply(*strip, value); };

        automation->addBinding(std::move(binding));
    };

    const auto findLane = [&](EntityId id) -> const AutomationLane* {
        for (const AutomationLane& lane : project.automation())
            if (lane.id == id)
                return &lane;
        return nullptr;
    };

    // A lane placed anywhere — as an automation clip, or through a pattern
    // that lists it — plays only through its placements. Only an unplaced
    // lane plays globally (the 11a behaviour, and what a lane freshly
    // written by automation recording does until it is arranged).
    std::unordered_map<EntityId, bool> lanePlaced;

    if (options.source == PlaybackSource::arrangement) {
        for (const Clip& clip : project.clips()) {
            // A muted placement still counts as PLACED: muting the clip must
            // silence the lane, not promote it back to playing everywhere.
            const Track* track   = project.findTrack(clip.track);
            const bool   audible = !clip.muted && (track == nullptr || !track->muted);

            const auto place = [&](EntityId laneId) {
                const AutomationLane* lane = findLane(laneId);
                if (lane == nullptr)
                    return;

                lanePlaced[lane->id] = true;

                if (!audible)
                    return;

                LanePlacement placement;
                placement.offsetTicks = clip.startTick - clip.sourceOffsetTicks;
                placement.windowStart = clip.startTick;
                placement.windowEnd   = clip.startTick + clip.lengthTicks;
                bindLane(*lane, placement);
            };

            if (clip.type == ClipType::automation) {
                place(clip.source);
            } else if (clip.type == ClipType::pattern) {
                if (const Pattern* pattern = project.findPattern(clip.source))
                    for (const EntityId laneId : pattern->automationLanes)
                        place(laneId);
            }
        }
    }

    for (const AutomationLane& lane : project.automation())
        if (!lanePlaced[lane.id])
            bindLane(lane, LanePlacement{});

    engine::AutomationNode* automationHandle = nullptr;

    if (automation->bindingCount() > 0) {
        automationHandle = automation.get();
        builder.addNode(std::move(automation));
    }

    builder.setMaster(master);

    compiled.graph = builder.compile(options.sampleRate, options.maxBlockSize, options.channelCount);
    if (compiled.graph == nullptr) {
        compiled.error = builder.lastError();
        return compiled;
    }

    compiled.channels      = std::move(channelIds);
    compiled.instruments   = std::move(instrumentNodes);
    compiled.channelStrips = std::move(channelStripNodes);
    compiled.mixerNodes    = std::move(mixerIds);
    compiled.strips        = std::move(stripNodes);
    compiled.automation    = automationHandle;
    return compiled;
}

} // namespace incdaw::project
