#include "project/ProjectGraphCompiler.h"

#include "engine/dsp/GainNode.h"
#include "engine/instrument/SimpleSynth.h"
#include "project/PatternCompiler.h"

#include <algorithm>

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

    engine::GraphBuilder builder;

    // The master exists even with no audible channel. A graph without one would
    // fail to compile, and the user would get silence plus an error for the
    // entirely ordinary act of muting everything.
    const auto master = builder.addNode(std::make_unique<engine::dsp::GainNode>(options.masterGain));

    std::vector<engine::InstrumentNode*> instrumentNodes;
    std::vector<EntityId>                channelIds;

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

        // Channel volume gets its own node rather than being folded into the
        // instrument: it is a mixer-side property, it has to be automatable
        // (Phase 11), and GainNode already smooths changes so a volume move
        // does not click.
        const auto gain = builder.addNode(std::make_unique<engine::dsp::GainNode>(
            static_cast<engine::Sample>(channel.volume)));

        builder.connect(source, gain);
        builder.connect(gain, master);

        channelIds.push_back(channel.id);
        instrumentNodes.push_back(handle);
    }

    builder.setMaster(master);

    compiled.graph = builder.compile(options.sampleRate, options.maxBlockSize, options.channelCount);
    if (compiled.graph == nullptr) {
        compiled.error = builder.lastError();
        return compiled;
    }

    compiled.channels    = std::move(channelIds);
    compiled.instruments = std::move(instrumentNodes);
    return compiled;
}

} // namespace incdaw::project
