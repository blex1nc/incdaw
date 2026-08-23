#pragma once

#include "engine/audio/AudioStream.h"
#include "engine/audio/SampleCache.h"
#include "engine/core/SampleRingBuffer.h"
#include "engine/graph/ParameterSink.h"
#include "engine/graph/RenderGraph.h"
#include "engine/graph/StateIO.h"
#include "engine/instrument/Instrument.h"
#include "engine/automation/AutomationNode.h"
#include "engine/performance/PerformanceScheduler.h"
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

/// Builds the render-graph node for one insert slot.
///
/// Injected for the same reason `InstrumentFactory` is, and for one more:
/// `project/` must never include a CLAP header (docs/DECISIONS.md D-028). The
/// factory hands back an `engine::Node`, so the compiler can wire a hosted
/// plugin into the graph while knowing nothing about plugin formats.
///
/// Returning nullptr makes the slot a pass-through; `error` is then recorded
/// as a compile warning. A plugin that will not load must not stop the project
/// from playing.
using InsertFactory =
    std::function<std::unique_ptr<engine::Node>(const PluginSlot& slot, std::string& error)>;

struct GraphCompileOptions {
    engine::SampleRate sampleRate   = 48000.0;
    engine::FrameCount maxBlockSize = 512;
    std::size_t        channelCount = 2;

    /// Audio assets longer than this stream from disk instead of being
    /// preloaded whole (needs `diskStreamer` set; without one everything
    /// preloads). 30 seconds of 48 kHz stereo is ~11 MB decoded — an
    /// arbitrary but generous line between "a take" and "an hour of audio".
    engine::FrameCount streamingThresholdFrames = 48000ll * 30;

    /// The service that keeps streamed clips' windows filled. Owned by the
    /// application (or the test); graphs only weak-reference it via the
    /// streams they register.
    engine::DiskStreamer* diskStreamer = nullptr;

    /// Decoded audio shared across rebuilds (docs/DECISIONS.md D-032): sampler
    /// zones and preloaded clips resolve through this when set. Null decodes
    /// fresh each compile, which is what most tests want. Owned by the
    /// application, like the streamer.
    engine::SampleCache* sampleCache = nullptr;

    /// Frames of a streamed sampler zone kept resident (the "head") so a note
    /// starts instantly while the disk window catches up. ~1.4 s at 48 kHz.
    /// Tests shrink it to force the hand-over quickly.
    engine::FrameCount samplerHeadFrames = 65536;

    /// The metronome: a click on every beat, mixed into the master. Session
    /// state rather than project data — like input monitoring, the toggle is a
    /// topology change and takes effect on the rebuild it causes.
    bool metronomeEnabled = false;

    /// Input monitoring: when set, an InputMonitorNode draining this ring
    /// feeds the master strip. The ring is the engine's and outlives every
    /// graph; `monitorChannelCount` is the input channel count at compile
    /// time (the ring itself is channel-agnostic).
    engine::SampleRingBuffer* monitorRing         = nullptr;
    std::size_t               monitorChannelCount = 0;

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

    /// Builds mixer insert nodes. Unset means every insert slot is a
    /// pass-through — which is what a build without plugin hosting, and every
    /// test that does not care about it, wants.
    InsertFactory      insertFactory;

    /// The parameter system automation resolves keys against. Null uses the
    /// built-ins ("volume", "pan"); tests and later phases register more.
    const ParameterRegistry* parameters = nullptr;

    /// Whether the clips before the arrangement's start marker are TRIGGERED
    /// rather than played in sequence (docs/PERFORMANCE_MODE.md).
    ///
    /// Off by default and therefore off for every project that has not asked
    /// for it: a start marker on its own changes nothing until the mode is
    /// switched on, which is what keeps this feature out of the way of songs
    /// that will never use it.
    bool               performanceMode = false;
};

/// A compiled graph plus the handles needed to drive it.
struct CompiledProjectGraph {
    std::unique_ptr<engine::CompiledGraph> graph;

    /// The tempo map this graph renders against — its own copy, pointed at by
    /// every node that converts ticks to frames. Lives exactly as long as the
    /// graph does, which is the whole point (see `compileProjectGraph`).
    std::unique_ptr<engine::TempoMap> tempoMap;

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

    /// The performance scene table, or nullptr when the graph was not compiled
    /// for Performance Mode. Owned here rather than by `graph` because the UI
    /// posts triggers into it and therefore needs to reach it.
    ///
    /// The nodes hold a raw pointer to it, so it must outlive them: it is
    /// declared before `graph` above only in reading order — destruction order
    /// within a struct is reverse declaration order, and `graph` is declared
    /// first, so it is destroyed first. That is the order this needs.
    std::unique_ptr<engine::PerformanceScheduler> performance;

    /// The playlist tracks the scheduler's slots correspond to, in slot order.
    std::vector<EntityId> performanceTracks;

    /// The automation evaluator, or nullptr when no lane compiled. Owned by
    /// `graph`, like everything else here.
    engine::AutomationNode* automation = nullptr;

    /// Insert slots that made it into the graph, and their state carriers —
    /// how project save reaches a hosted plugin's opaque blob
    /// (docs/PLUGIN_HOST.md §6). Bypassed and unbuildable slots are absent;
    /// so is a slot whose node carries no state. Owned by `graph`.
    std::vector<EntityId>         insertSlots;
    std::vector<engine::StateIO*> insertStates;

    /// Insert parameter sinks, same population rule (a slot whose node has no
    /// sink is absent) — what a parameter panel writes through. Owned by
    /// `graph`; the compiler's own automation and MIDI bindings resolve
    /// against the same sinks, so a panel and a lane cannot disagree.
    std::vector<EntityId>               sinkSlots;
    std::vector<engine::ParameterSink*> insertSinks;

    /// Every insert node that made it into the graph, by slot — the UI's
    /// door to effect-specific surfaces (the analyzer's spectrum). Owned by
    /// `graph`, like everything here.
    std::vector<EntityId>      builtSlots;
    std::vector<engine::Node*> builtInserts;

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

    /// State carrier for an insert slot, or nullptr if it is not in the graph.
    [[nodiscard]] engine::StateIO* insertStateFor(EntityId slot) const noexcept;

    /// Parameter sink for an insert slot, or nullptr if it is not in the graph.
    [[nodiscard]] engine::ParameterSink* insertSinkFor(EntityId slot) const noexcept;

    /// The insert node itself, or nullptr if it is not in the graph.
    [[nodiscard]] engine::Node* insertNodeFor(EntityId slot) const noexcept;
};

/// Compiles `project` against a COPY of `tempoMap`, owned by the returned
/// graph (`CompiledProjectGraph::tempoMap`).
///
/// The copy is what makes a tempo edit safe. The nodes that convert between
/// ticks and frames hold a pointer to the map for as long as they render, so
/// a map they share with the transport could not be rewritten while the audio
/// thread was inside it — and a vector reallocating under a binary search is
/// not a glitch, it is a crash. A tempo change is a graph rebuild instead: the
/// new graph carries the new map, and the old one keeps the map it was
/// compiled against until it is retired.
[[nodiscard]] CompiledProjectGraph compileProjectGraph(const Project&          project,
                                                       const engine::TempoMap& tempoMap,
                                                       const GraphCompileOptions& options);

} // namespace incdaw::project
