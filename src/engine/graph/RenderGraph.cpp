#include "engine/graph/RenderGraph.h"

#include "engine/core/RealtimeGuard.h"

#include <algorithm>
#include <deque>

namespace incdaw::engine {

// ── CompiledGraph ─────────────────────────────────────────────────────────────

void CompiledGraph::process(const AudioBufferView& output, FrameCount frameCount,
                            FramePosition playPosition, const MidiBuffer* liveMidi) noexcept
{
    if (!hasMaster_ || frameCount <= 0)
        return;

    if (frameCount > maxBlockSize_)
        frameCount = maxBlockSize_;   // never write past what was allocated

    for (const Step& step : steps_) {
        AudioBufferView stepOutput = pool_.buffer(step.outputBuffer).subBlock(0, frameCount);

        // Silence first: a node that writes nothing must produce silence, not
        // whatever the previous block left in this buffer.
        stepOutput.clear();

        const std::size_t inputCount = step.inputBuffers.size();
        for (std::size_t index = 0; index < inputCount; ++index)
            inputViews_[index] = pool_.buffer(step.inputBuffers[index]).subBlock(0, frameCount);

        ProcessContext context;
        context.output       = stepOutput;
        context.inputs       = inputViews_.data();
        context.inputCount   = inputCount;
        context.frameCount   = frameCount;
        context.sampleRate   = sampleRate_;
        context.playPosition = playPosition;
        context.liveMidi     = liveMidi;

        step.node->process(context);
    }

    output.copyFrom(pool_.buffer(masterBuffer_).subBlock(0, frameCount));
}

// ── GraphBuilder ──────────────────────────────────────────────────────────────

NodeIndex GraphBuilder::addNode(std::unique_ptr<Node> node)
{
    if (node == nullptr)
        return invalidNode;

    nodes_.push_back(std::move(node));
    return nodes_.size() - 1;
}

void GraphBuilder::connect(NodeIndex source, NodeIndex destination)
{
    connections_.push_back({source, destination});
}

std::unique_ptr<CompiledGraph> GraphBuilder::compile(SampleRate sampleRate, FrameCount maxBlockSize,
                                                     std::size_t channelCount)
{
    error_.clear();

    const std::size_t count = nodes_.size();

    if (count == 0)                                  { error_ = "graph has no nodes"; return nullptr; }
    if (master_ == invalidNode || master_ >= count)  { error_ = "no valid master node"; return nullptr; }
    if (sampleRate <= 0.0)                           { error_ = "sample rate must be positive"; return nullptr; }
    if (maxBlockSize <= 0)                           { error_ = "block size must be positive"; return nullptr; }
    if (channelCount == 0)                           { error_ = "channel count must be positive"; return nullptr; }

    for (const Connection& connection : connections_) {
        if (connection.source >= count || connection.destination >= count) {
            error_ = "connection references a node that does not exist";
            return nullptr;
        }
        if (connection.source == connection.destination) {
            error_ = "a node cannot be connected to itself";
            return nullptr;
        }
    }

    // ── Topological sort (Kahn) ───────────────────────────────────────────────
    std::vector<std::vector<NodeIndex>> successors(count);
    std::vector<std::size_t>            remainingInputs(count, 0);
    std::vector<std::vector<NodeIndex>> sources(count);

    for (const Connection& connection : connections_) {
        successors[connection.source].push_back(connection.destination);
        sources[connection.destination].push_back(connection.source);
        ++remainingInputs[connection.destination];
    }

    std::deque<NodeIndex>  ready;
    std::vector<NodeIndex> order;
    order.reserve(count);

    for (NodeIndex index = 0; index < count; ++index)
        if (remainingInputs[index] == 0)
            ready.push_back(index);

    while (!ready.empty()) {
        const NodeIndex current = ready.front();
        ready.pop_front();
        order.push_back(current);

        for (const NodeIndex successor : successors[current])
            if (--remainingInputs[successor] == 0)
                ready.push_back(successor);
    }

    if (order.size() != count) {
        // Kahn's algorithm stalls exactly when a cycle exists. Rejecting is the
        // only honest outcome: breaking an arbitrary edge would render a graph
        // that is not the one the user built.
        error_ = "the connections contain a cycle";
        return nullptr;
    }

    // ── Latency accumulation ──────────────────────────────────────────────────
    // Longest path to each node, in frames. Full delay compensation (inserting
    // delay lines on the shorter paths) arrives with the mixer in Phase 10;
    // what the engine needs now is the correct total to report to the device
    // and to the recording path.
    std::vector<FrameCount> latencyTo(count, 0);

    for (const NodeIndex index : order) {
        FrameCount incoming = 0;
        for (const NodeIndex source : sources[index])
            incoming = std::max(incoming, latencyTo[source]);

        latencyTo[index] = incoming + nodes_[index]->latencyFrames();
    }

    // ── Buffer assignment ─────────────────────────────────────────────────────
    // One buffer per node output. Buffer reuse (handing a node the buffer of a
    // source whose last consumer it is) would cut the working set materially on
    // large graphs, but it is an optimisation, and docs/PERFORMANCE.md §4
    // forbids optimising before measuring. Correct and obvious first.
    auto graph = std::make_unique<CompiledGraph>();

    graph->pool_.allocate(count, channelCount, maxBlockSize);
    graph->sampleRate_   = sampleRate;
    graph->maxBlockSize_ = maxBlockSize;
    graph->totalLatency_ = latencyTo[master_];
    graph->masterBuffer_ = master_;
    graph->hasMaster_    = true;
    graph->order_        = order;

    std::size_t widestInput = 0;

    graph->steps_.reserve(count);
    for (const NodeIndex index : order) {
        CompiledGraph::Step step;
        step.node         = nodes_[index].get();
        step.outputBuffer = index;
        step.inputBuffers = sources[index];

        widestInput = std::max(widestInput, step.inputBuffers.size());
        graph->steps_.push_back(std::move(step));
    }

    // Scratch for input views, sized once so that `process` never allocates.
    graph->inputViews_.resize(widestInput);

    for (auto& node : nodes_)
        node->prepare(sampleRate, maxBlockSize);

    graph->nodes_ = std::move(nodes_);

    // The builder has given away its nodes; leave it in a defined empty state
    // rather than one that looks reusable but is full of dangling indices.
    nodes_.clear();
    connections_.clear();
    master_ = invalidNode;

    return graph;
}

} // namespace incdaw::engine
