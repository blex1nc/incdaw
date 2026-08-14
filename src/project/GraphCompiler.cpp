#include "project/GraphCompiler.h"

#include "engine/dsp/ChannelStripNode.h"
#include "engine/dsp/GainNode.h"

#include <unordered_map>
#include "engine/instrument/InstrumentNode.h"
#include "project/InstrumentFactory.h"
#include "project/PatternCompiler.h"

#include <algorithm>

namespace incdaw::project {
namespace {

/// Whether this channel is heard, given the project's solo state.
///
/// Solo is exclusive across the project rather than a per-channel flag: as soon
/// as anything is soloed, everything unsoloed is silent, whatever its own mute
/// says. Its own mute still applies — soloing a muted channel does not unmute
/// it, which is what lets you solo a group without losing the mutes inside it.
[[nodiscard]] bool channelIsAudible(const Channel& channel, bool anySoloed) noexcept
{
    if (channel.muted)
        return false;

    return !anySoloed || channel.soloed;
}

/// The two nodes a mixer strip is built from.
///
/// Split in two on purpose. `input` is where everything feeding this strip is
/// summed and where the insert chain will hang (Phase 13); `strip` is the
/// fader, pan, polarity and meter. A pre-fader send has to tap the signal
/// between them, and with a single node there would be nowhere to tap.
struct MixerStrip {
    engine::NodeIndex input = engine::invalidNode;
    engine::NodeIndex strip = engine::invalidNode;
};

} // namespace

CompiledProjectGraph compileProjectGraph(const Project& project, const engine::TempoMap& tempoMap,
                                         const GraphCompileOptions& options)
{
    CompiledProjectGraph result;

    engine::GraphBuilder builder;

    // ── Mixer ─────────────────────────────────────────────────────────────────
    //
    // Built before the channels, because a channel has to be routed into its
    // mixer node and cannot be until the node exists.
    const bool anyMixerSoloed = [&project] {
        for (const MixerNode& node : project.mixerNodes())
            if (node.soloed)
                return true;

        return false;
    }();

    std::unordered_map<EntityId, MixerStrip> mixerStrips;

    for (const MixerNode& node : project.mixerNodes()) {
        auto strip = std::make_unique<engine::dsp::ChannelStripNode>();
        strip->setVolume(static_cast<engine::Sample>(node.volume));
        strip->setPan(static_cast<engine::Sample>(node.pan));
        strip->setPolarityFlipped(node.polarityFlip);

        // The master is never silenced by solo. Soloing a track must not mute
        // the output everything is going through.
        const bool exemptFromSolo = node.type == MixerNodeType::master;
        strip->setMuted(node.muted || (anyMixerSoloed && !node.soloed && !exemptFromSolo));

        engine::dsp::ChannelStripNode* stripPointer = strip.get();

        MixerStrip built;
        built.input = builder.addNode(std::make_unique<engine::dsp::GainNode>(1.0f));
        built.strip = builder.addNode(std::move(strip));
        builder.connect(built.input, built.strip);

        mixerStrips.emplace(node.id, built);
        result.mixerOrder.push_back(node.id);
        result.mixerStripNodes.push_back(stripPointer);
    }

    const auto masterStrip = mixerStrips.find(project.masterMixerNode());

    // The master gain exists whether or not anything reaches it. A graph with no
    // master cannot compile, and a project with no channels must still render
    // silence rather than fail to start the device.
    const auto master = builder.addNode(std::make_unique<engine::dsp::GainNode>(options.masterGain));

    if (masterStrip != mixerStrips.end())
        builder.connect(masterStrip->second.strip, master);

    // ── Routing ───────────────────────────────────────────────────────────────
    std::unordered_map<EntityId, bool> hasOutgoing;

    for (const RoutingConnection& link : project.routing()) {
        const auto source      = mixerStrips.find(link.source);
        const auto destination = mixerStrips.find(link.destination);

        if (source == mixerStrips.end() || destination == mixerStrips.end())
            continue;   // a routing entry naming an entity that no longer exists

        // Sidechain feeds go to a plugin's key input, and there are no plugins
        // yet (Phase 13). Adding the edge anyway would sum the sidechain signal
        // into the destination's main input — audibly wrong, and exactly the
        // kind of "it's wired up" that is worse than not wired at all.
        if (link.sidechain)
            continue;

        // Pre-fader taps the summing point, post-fader the strip's output. That
        // is the whole difference, and it is why the strip is two nodes.
        const engine::NodeIndex tap = link.preFader ? source->second.input : source->second.strip;

        engine::NodeIndex from = tap;

        if (link.gain != 1.0) {
            from = builder.addNode(
                std::make_unique<engine::dsp::GainNode>(static_cast<engine::Sample>(link.gain)));
            builder.connect(tap, from);
        }

        builder.connect(from, destination->second.input);

        // A send is an extra path, not a replacement for the main one: a track
        // with a reverb send still reaches its bus. Only a non-send edge counts
        // as "this node has somewhere to go".
        if (!link.isSend)
            hasOutgoing[link.source] = true;
    }

    // Anything with no explicit output goes to the master, which is what a mixer
    // does by default and what every project written before routing existed
    // means.
    for (const MixerNode& node : project.mixerNodes()) {
        if (node.id == project.masterMixerNode() || hasOutgoing[node.id])
            continue;

        const auto strip = mixerStrips.find(node.id);
        if (strip == mixerStrips.end() || masterStrip == mixerStrips.end())
            continue;

        builder.connect(strip->second.strip, masterStrip->second.input);
    }

    const Pattern* activePattern = options.activePattern.isValid()
                                       ? project.findPattern(options.activePattern)
                                       : (project.patterns().empty() ? nullptr : &project.patterns().front());

    const bool anySoloed = project.anyChannelSoloed();
    const EntityId defaultChannel = project.defaultChannel();

    for (const Channel& channel : project.channels()) {
        auto instrument = createInstrument(channel.instrument);

        auto instrumentNode = std::make_unique<engine::InstrumentNode>(std::move(instrument), tempoMap);

        // Note sourcing is the only thing the playback mode changes. Both paths
        // compile the same patterns through the same code, so a pattern cannot
        // sound one way in pattern mode and another in song mode.
        if (options.mode == PlaybackMode::song) {
            compileArrangementInto(instrumentNode->sequence(), project, channel.id, options.randomSeed);
        } else if (activePattern != nullptr) {
            PatternCompileOptions patternOptions;
            patternOptions.channel        = channel.id;
            patternOptions.defaultChannel = defaultChannel;
            patternOptions.randomSeed     = options.randomSeed ^ activePattern->id.value();
            compilePatternInto(instrumentNode->sequence(), *activePattern, patternOptions);
        }

        auto strip = std::make_unique<engine::dsp::ChannelStripNode>();
        strip->setVolume(static_cast<engine::Sample>(channel.volume));
        strip->setPan(static_cast<engine::Sample>(channel.pan));
        strip->setMuted(!channelIsAudible(channel, anySoloed));

        engine::InstrumentNode*        instrumentPointer = instrumentNode.get();
        engine::dsp::ChannelStripNode* stripPointer      = strip.get();

        const auto instrumentIndex = builder.addNode(std::move(instrumentNode));
        const auto stripIndex      = builder.addNode(std::move(strip));

        builder.connect(instrumentIndex, stripIndex);

        // Into the channel's mixer node, or straight to the master when it names
        // one that no longer exists. A channel whose mixer track was deleted
        // must not go silent — that is data loss the user cannot see.
        const auto destination = mixerStrips.find(channel.outputMixerNode);

        if (destination != mixerStrips.end())
            builder.connect(stripIndex, destination->second.input);
        else if (masterStrip != mixerStrips.end())
            builder.connect(stripIndex, masterStrip->second.input);
        else
            builder.connect(stripIndex, master);

        result.channelOrder.push_back(channel.id);
        result.instrumentNodes.push_back(instrumentPointer);
        result.stripNodes.push_back(stripPointer);
    }

    builder.setMaster(master);

    result.lengthTicks = options.mode == PlaybackMode::song
                             ? arrangementLengthTicks(project)
                             : (activePattern != nullptr ? activePattern->length : 0);

    result.graph = builder.compile(options.sampleRate, options.maxBlockSize, options.channelCount);
    result.compensationNodes = builder.compensationNodesInserted();

    if (result.graph == nullptr) {
        result.error = builder.lastError();
        result.channelOrder.clear();
        result.instrumentNodes.clear();
        result.stripNodes.clear();
        result.mixerOrder.clear();
        result.mixerStripNodes.clear();
    }

    return result;
}

} // namespace incdaw::project
