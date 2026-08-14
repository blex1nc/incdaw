#pragma once

#include "engine/graph/RenderGraph.h"
#include "engine/instrument/Instrument.h"
#include "engine/automation/AutomationNode.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/instrument/InstrumentNode.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/ParameterRegistry.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::project {

/// Turns a Project into a renderable graph.
///
/// This is the seam between the model and the engine, and it lives in
/// `project/` because it has to: `engine/` sits below and cannot see a Project.
/// Before this existed the UI assembled the graph by hand from one hardcoded
/// pattern, which meant every new subsystem would have had to be threaded
/// through the UI as well.
///
/// The signal path is:
///
///     instrument -> channel strip -> mixer track -> [sends/buses] -> master
///
/// Every one of those is a MixerStripNode, so channel volume, channel pan,
/// mixer volume, pan, mute and polarity are all the same well-tested arithmetic
/// rather than four approximations of it.

enum class PlaybackSource : std::uint8_t {
    /// One pattern on loop — what the Piano Roll edits.
    pattern,
    /// The arrangement's pattern clips.
    arrangement,
};

/// Builds the instrument for a channel.
///
/// Injected rather than hardcoded so that plugin hosting (Phase 13) becomes a
/// different factory instead of a change to this file. Returning nullptr leaves
/// the channel silent, which is the correct behaviour for a channel whose
/// plugin is missing.
using InstrumentFactory = std::function<std::unique_ptr<engine::Instrument>(const Channel&)>;

/// The built-in factory: every channel gets a SimpleSynth, the only instrument
/// INCDAW currently has.
[[nodiscard]] InstrumentFactory defaultInstrumentFactory();

struct GraphCompileOptions {
    engine::SampleRate sampleRate   = 48000.0;
    engine::FrameCount maxBlockSize = 512;
    std::size_t        channelCount = 2;

    /// Applied at the master strip, on top of the master mixer node's own
    /// volume. Headroom for a project that has not been mixed yet, not a
    /// substitute for the master fader.
    engine::Sample     masterGain   = engine::Sample{0.8f};

    PlaybackSource     source       = PlaybackSource::pattern;

    /// Which pattern plays in `PlaybackSource::pattern`. Unset means the
    /// project's first pattern.
    EntityId           pattern{};

    std::uint64_t      randomSeed   = 0;

    InstrumentFactory  instrumentFactory;

    /// The parameter system automation resolves keys against. Null uses the
    /// built-ins ("volume", "pan"); tests and later phases register more.
    const ParameterRegistry* parameters = nullptr;
};

/// A compiled graph plus the handles needed to drive it.
struct CompiledProjectGraph {
    std::unique_ptr<engine::CompiledGraph> graph;

    /// Channels that made it into the graph, and their instrument nodes. Muted
    /// channels are absent: they are left out of the graph entirely rather than
    /// rendered and multiplied by zero, so a muted channel costs nothing.
    ///
    /// The pointers are owned by `graph` and are valid only while it lives.
    std::vector<EntityId>                channels;
    std::vector<engine::InstrumentNode*> instruments;

    /// Mixer nodes that made it into the graph, and their strips. The UI reads
    /// meters through these, and writes fader, pan and mute moves straight to
    /// them: a parameter change does not alter the topology, so recompiling for
    /// one would be waste — and would reset every meter on the way past.
    std::vector<EntityId>                    mixerNodes;
    std::vector<engine::dsp::MixerStripNode*> strips;

    /// Channel strips, in the same order as `channels`.
    std::vector<engine::dsp::MixerStripNode*> channelStrips;

    /// The automation evaluator, or nullptr when no lane compiled. Owned by
    /// `graph`, like everything else here.
    engine::AutomationNode* automation = nullptr;

    std::string error;

    /// Non-fatal compile notes: an asset file that could not be read, an
    /// asset at the wrong sample rate. The graph still compiled; the affected
    /// clips are silent, and the UI should say why rather than play wrong.
    std::vector<std::string> warnings;

    [[nodiscard]] explicit operator bool() const noexcept { return graph != nullptr; }

    /// Instrument node for a channel, or nullptr if it is not in the graph.
    [[nodiscard]] engine::InstrumentNode* instrumentFor(EntityId channel) const noexcept;

    /// Strip for a mixer node, or nullptr if it is not in the graph.
    [[nodiscard]] engine::dsp::MixerStripNode* stripFor(EntityId mixerNode) const noexcept;

    /// Strip for a channel, or nullptr if the channel is silent.
    [[nodiscard]] engine::dsp::MixerStripNode* channelStripFor(EntityId channel) const noexcept;
};

/// Compiles `project` against `tempoMap`, which must outlive the returned graph
/// — the instrument nodes hold a reference to it.
[[nodiscard]] CompiledProjectGraph compileProjectGraph(const Project&          project,
                                                       const engine::TempoMap& tempoMap,
                                                       const GraphCompileOptions& options);

} // namespace incdaw::project
