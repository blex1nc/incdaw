#pragma once

#include "engine/core/AudioBufferPool.h"
#include "engine/graph/Node.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::engine {

/// Identifies a node within one graph.
using NodeIndex = std::size_t;

inline constexpr NodeIndex invalidNode = static_cast<NodeIndex>(-1);

/// An immutable, ready-to-render graph.
///
/// The audio thread only ever sees this type, and only ever reads it
/// (docs/ARCHITECTURE.md §7). Everything expensive — topological sort, cycle
/// detection, buffer allocation, latency accumulation — happened in
/// `GraphBuilder::compile` on a non-realtime thread.
///
/// Editing a graph means building a new one and swapping it in atomically. The
/// old one goes to the reaper; it is never mutated in place, because the audio
/// thread may be halfway through reading it.
class CompiledGraph {
public:
    CompiledGraph() = default;

    /// Renders one block into `output`. Realtime-safe.
    ///
    /// `output` is the device's buffer. The graph renders into its own pool and
    /// copies the master node's result out at the end: one extra copy per
    /// block, in exchange for nodes never needing to know whether they happen
    /// to be the master. Measured against the cost of a block, it does not
    /// register; if it ever does, the master node can be given the device
    /// buffer directly.
    void process(const AudioBufferView& output, FrameCount frameCount, FramePosition playPosition) noexcept;

    [[nodiscard]] std::size_t nodeCount()     const noexcept { return nodes_.size(); }
    [[nodiscard]] FrameCount  latencyFrames() const noexcept { return totalLatency_; }
    [[nodiscard]] SampleRate  sampleRate()    const noexcept { return sampleRate_; }
    [[nodiscard]] FrameCount  maxBlockSize()  const noexcept { return maxBlockSize_; }

    /// Node order as the audio thread will execute it. Exposed for tests and
    /// graph dumps.
    [[nodiscard]] const std::vector<NodeIndex>& executionOrder() const noexcept { return order_; }

private:
    friend class GraphBuilder;

    struct Step {
        Node*                    node = nullptr;
        std::size_t              outputBuffer = 0;
        std::vector<std::size_t> inputBuffers;
    };

    std::vector<std::unique_ptr<Node>> nodes_;
    std::vector<Step>                  steps_;        ///< in execution order
    std::vector<NodeIndex>             order_;
    std::vector<AudioBufferView>       inputViews_;   ///< scratch, sized at compile time

    AudioBufferPool pool_;
    std::size_t     masterBuffer_  = 0;
    FrameCount      totalLatency_  = 0;
    SampleRate      sampleRate_    = 0.0;
    FrameCount      maxBlockSize_  = 0;
    bool            hasMaster_     = false;
};

/// Builds a graph. Not realtime-safe; allocates freely.
class GraphBuilder {
public:
    GraphBuilder() = default;

    /// Takes ownership. Returns the index used to connect it.
    NodeIndex addNode(std::unique_ptr<Node> node);

    /// Routes `source`'s output into `destination`'s inputs. A node may have
    /// any number of sources; it receives them in connection order.
    void connect(NodeIndex source, NodeIndex destination);

    /// The node whose output reaches the device.
    void setMaster(NodeIndex node) noexcept { master_ = node; }

    [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }

    /// Reason the last `compile` failed. Empty on success.
    [[nodiscard]] const std::string& lastError() const noexcept { return error_; }

    /// Produces a renderable graph, or nullptr on failure.
    ///
    /// Fails when: no master is set, an index is out of range, or the
    /// connections contain a cycle. A cycle is rejected rather than broken
    /// arbitrarily — silently cutting one would produce a graph that renders
    /// but is not the one the user built.
    [[nodiscard]] std::unique_ptr<CompiledGraph> compile(SampleRate sampleRate, FrameCount maxBlockSize,
                                                         std::size_t channelCount);

private:
    struct Connection {
        NodeIndex source      = invalidNode;
        NodeIndex destination = invalidNode;
    };

    std::vector<std::unique_ptr<Node>> nodes_;
    std::vector<Connection>            connections_;
    NodeIndex                          master_ = invalidNode;
    std::string                        error_;
};

} // namespace incdaw::engine
