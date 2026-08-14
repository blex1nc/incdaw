#pragma once

#include "engine/graph/RenderGraph.h"
#include "engine/instrument/Instrument.h"
#include "engine/instrument/InstrumentNode.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"

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
/// The mixer is deliberately absent. Channel volume is applied, pan is not:
/// a pan law belongs to the mixer (Phase 10), and an approximation here would
/// have to be unpicked later. The signal path is channel -> master.

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

    /// Applied at the master node. A placeholder for the master mixer track
    /// until Phase 10 builds one.
    engine::Sample     masterGain   = engine::Sample{0.8f};

    PlaybackSource     source       = PlaybackSource::pattern;

    /// Which pattern plays in `PlaybackSource::pattern`. Unset means the
    /// project's first pattern.
    EntityId           pattern{};

    std::uint64_t      randomSeed   = 0;

    InstrumentFactory  instrumentFactory;
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

    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return graph != nullptr; }

    /// Instrument node for a channel, or nullptr if it is not in the graph.
    [[nodiscard]] engine::InstrumentNode* instrumentFor(EntityId channel) const noexcept;
};

/// Compiles `project` against `tempoMap`, which must outlive the returned graph
/// — the instrument nodes hold a reference to it.
[[nodiscard]] CompiledProjectGraph compileProjectGraph(const Project&          project,
                                                       const engine::TempoMap& tempoMap,
                                                       const GraphCompileOptions& options);

} // namespace incdaw::project
