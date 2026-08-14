#include "project/ProjectGraphCompiler.h"

#include "engine/dsp/GainNode.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/instrument/SimpleSynth.h"
#include "project/PatternCompiler.h"

#include <algorithm>
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

    // ── Routing between mixer nodes ─────────────────────────────────────────
    for (const RoutingConnection& connection : project.routing()) {
        const auto source      = stripIndices.find(connection.source);
        const auto destination = stripIndices.find(connection.destination);

        if (source == stripIndices.end() || destination == stripIndices.end())
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

        const auto destination = stripIndices.find(channel.outputMixerNode);

        builder.connect(source, strip);
        builder.connect(strip, destination != stripIndices.end() ? destination->second : master);

        channelIds.push_back(channel.id);
        instrumentNodes.push_back(handle);
        channelStripNodes.push_back(channelHandle);
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
    return compiled;
}

} // namespace incdaw::project
