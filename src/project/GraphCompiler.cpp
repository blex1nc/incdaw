#include "project/GraphCompiler.h"

#include "engine/dsp/ChannelStripNode.h"
#include "engine/dsp/GainNode.h"
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

} // namespace

CompiledProjectGraph compileProjectGraph(const Project& project, const engine::TempoMap& tempoMap,
                                         const GraphCompileOptions& options)
{
    CompiledProjectGraph result;

    engine::GraphBuilder builder;

    // The master exists whether or not anything reaches it. A graph with no
    // master cannot compile, and a project with no channels must still render
    // silence rather than fail to start the device.
    const auto master = builder.addNode(std::make_unique<engine::dsp::GainNode>(options.masterGain));

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

    if (result.graph == nullptr) {
        result.error = builder.lastError();
        result.channelOrder.clear();
        result.instrumentNodes.clear();
        result.stripNodes.clear();
    }

    return result;
}

} // namespace incdaw::project
