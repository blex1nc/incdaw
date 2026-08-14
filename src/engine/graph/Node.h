#pragma once

#include "engine/core/AudioBuffer.h"
#include "engine/core/Time.h"
#include "engine/midi/MidiBuffer.h"

#include <cstddef>

namespace incdaw::engine {

/// Everything a node is allowed to know while rendering one block.
///
/// Passed by const reference into `Node::process`. Contains no pointers the
/// node may keep: every view is valid only for the duration of the call.
struct ProcessContext {
    /// Where this node must write its result. Already silenced by the graph, so
    /// a node with nothing to say may simply return.
    AudioBufferView output;

    /// Outputs of this node's sources, in the order they were connected.
    const AudioBufferView* inputs     = nullptr;
    std::size_t            inputCount = 0;

    FrameCount    frameCount = 0;
    SampleRate    sampleRate = 0.0;

    /// Timeline position of the first frame of this block.
    FramePosition playPosition = 0;

    /// MIDI arriving from hardware or the on-screen keyboard for this block,
    /// with offsets relative to it. Null when there is no live input.
    const MidiBuffer* liveMidi = nullptr;

    [[nodiscard]] AudioBufferView input(std::size_t index) const noexcept
    {
        return index < inputCount ? inputs[index] : AudioBufferView{};
    }
};

/// A unit of audio processing in the render graph.
///
/// `process` runs on the audio thread and is bound by the prime directive
/// (docs/AUDIO_ENGINE.md §1): no allocation, no locks, no I/O, no unbounded
/// work. `prepare` is where a node does all of that instead — it runs on a
/// non-realtime thread before the node is ever rendered.
class Node {
public:
    Node() = default;
    virtual ~Node() = default;

    Node(const Node&)            = delete;
    Node& operator=(const Node&) = delete;

    /// Called off the audio thread before rendering begins, and again whenever
    /// the sample rate or block size changes. May allocate.
    virtual void prepare(SampleRate sampleRate, FrameCount maxBlockSize) { (void)sampleRate; (void)maxBlockSize; }

    /// Renders one block. Realtime-safe.
    virtual void process(const ProcessContext& context) noexcept = 0;

    /// Processing delay this node introduces, in frames. Accumulated by the
    /// graph compiler so that delay compensation can align parallel paths
    /// (docs/AUDIO_ENGINE.md §7).
    [[nodiscard]] virtual FrameCount latencyFrames() const noexcept { return 0; }

    /// For diagnostics and graph dumps. Must be a string literal or otherwise
    /// outlive the node.
    [[nodiscard]] virtual const char* name() const noexcept { return "Node"; }
};

} // namespace incdaw::engine
