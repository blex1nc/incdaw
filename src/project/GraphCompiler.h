#pragma once

#include "engine/graph/RenderGraph.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::engine {
class InstrumentNode;
namespace dsp { class ChannelStripNode; }
} // namespace incdaw::engine

namespace incdaw::project {

/// What the transport plays.
///
/// The two-mode workflow is the reason a pattern-based DAW feels different from
/// a linear one: you build in pattern mode, hearing one pattern loop, and you
/// arrange in song mode, hearing the timeline. Both play the same patterns
/// through the same channels — only the note sources differ.
enum class PlaybackMode : std::uint8_t { pattern, song };

struct GraphCompileOptions {
    PlaybackMode       mode = PlaybackMode::pattern;

    /// The pattern the editor is showing. Played on loop in pattern mode;
    /// ignored in song mode.
    EntityId           activePattern;

    engine::SampleRate sampleRate   = 48000.0;
    engine::FrameCount maxBlockSize = 512;
    std::size_t        channelCount = 2;

    /// Applied at the master node. Not a mixer fader — the mixer arrives in
    /// Phase 10 and will sit between the channels and this.
    float              masterGain = 0.8f;

    std::uint64_t      randomSeed = 0;
};

/// A render graph built from a project, plus what the caller needs to drive it.
struct CompiledProjectGraph {
    std::unique_ptr<engine::CompiledGraph> graph;
    std::string                            error;

    /// Loop end for pattern mode, or the end of the arrangement in song mode,
    /// in ticks. 0 when there is nothing to play.
    Tick        lengthTicks = 0;

    /// Channel ids in the order their nodes were added, so that a caller can
    /// map a channel back to its strip for metering or live MIDI.
    std::vector<EntityId> channelOrder;

    /// Instrument nodes, parallel to `channelOrder`. Owned by `graph`; valid
    /// only for as long as it is.
    std::vector<engine::InstrumentNode*>          instrumentNodes;
    std::vector<engine::dsp::ChannelStripNode*>   stripNodes;

    [[nodiscard]] explicit operator bool() const noexcept { return graph != nullptr; }
};

/// Compiles a project into a render graph.
///
/// This is the seam between the model and the engine, and it is deliberately
/// one function: everything the audio thread will ever execute is decided here,
/// on a non-realtime thread, and installed with one atomic swap. Nothing
/// downstream mutates the graph — an edit produces a new one.
///
/// `tempoMap` must outlive the returned graph: the instrument nodes hold a
/// reference to it and read it from the audio thread.
[[nodiscard]] CompiledProjectGraph compileProjectGraph(const Project& project,
                                                       const engine::TempoMap& tempoMap,
                                                       const GraphCompileOptions& options);

} // namespace incdaw::project
