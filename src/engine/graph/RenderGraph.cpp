#include "engine/graph/RenderGraph.h"

#include "engine/dsp/DelayLineNode.h"

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

GraphBuilder::Topology GraphBuilder::analyse() const
{
    const std::size_t count = nodes_.size();

    Topology topology;
    topology.sources.assign(count, {});
    topology.latencyTo.assign(count, 0);
    topology.order.reserve(count);

    std::vector<std::vector<NodeIndex>> successors(count);
    std::vector<std::size_t>            remainingInputs(count, 0);

    for (const Connection& connection : connections_) {
        successors[connection.source].push_back(connection.destination);
        topology.sources[connection.destination].push_back(connection.source);
        ++remainingInputs[connection.destination];
    }

    std::deque<NodeIndex> ready;

    for (NodeIndex index = 0; index < count; ++index)
        if (remainingInputs[index] == 0)
            ready.push_back(index);

    while (!ready.empty()) {
        const NodeIndex current = ready.front();
        ready.pop_front();
        topology.order.push_back(current);

        for (const NodeIndex successor : successors[current])
            if (--remainingInputs[successor] == 0)
                ready.push_back(successor);
    }

    topology.acyclic = topology.order.size() == count;
    if (!topology.acyclic)
        return topology;

    // Longest path to each node, in frames: what a summing point downstream has
    // to wait for.
    for (const NodeIndex index : topology.order) {
        FrameCount incoming = 0;
        for (const NodeIndex source : topology.sources[index])
            incoming = std::max(incoming, topology.latencyTo[source]);

        topology.latencyTo[index] = incoming + nodes_[index]->latencyFrames();
    }

    return topology;
}

bool GraphBuilder::insertDelayCompensation(const Topology& topology)
{
    bool inserted = false;

    // Collected first, applied after: adding nodes while walking the edges
    // would invalidate the analysis this loop is reading.
    struct Compensation {
        std::size_t connection = 0;
        FrameCount  delay      = 0;
    };

    std::vector<Compensation> needed;

    for (NodeIndex destination = 0; destination < topology.sources.size(); ++destination) {
        const std::vector<NodeIndex>& sources = topology.sources[destination];
        if (sources.size() < 2)
            continue;   // a single source is trivially aligned with itself

        FrameCount latest = 0;
        for (const NodeIndex source : sources)
            latest = std::max(latest, topology.latencyTo[source]);

        for (std::size_t index = 0; index < connections_.size(); ++index) {
            const Connection& connection = connections_[index];
            if (connection.destination != destination)
                continue;

            const FrameCount delay = latest - topology.latencyTo[connection.source];
            if (delay > 0)
                needed.push_back({index, delay});
        }
    }

    for (const Compensation& compensation : needed) {
        // By index throughout: appending to `connections_` may reallocate it,
        // and a reference taken before the append would dangle.
        const NodeIndex source = connections_[compensation.connection].source;

        // source -> delay -> destination. The delay reports its own latency, so
        // the recomputed arrival time at the destination is the one every other
        // path already had.
        const NodeIndex delayIndex =
            addNode(std::make_unique<dsp::DelayLineNode>(compensation.delay));

        connections_.push_back({source, delayIndex});
        connections_[compensation.connection].source = delayIndex;

        inserted = true;
    }

    return inserted;
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

    // ── Topology, latency, and delay compensation ─────────────────────────────
    Topology topology = analyse();

    if (!topology.acyclic) {
        // Kahn's algorithm stalls exactly when a cycle exists. Rejecting is the
        // only honest outcome: breaking an arbitrary edge would render a graph
        // that is not the one the user built.
        error_ = "the connections contain a cycle";
        return nullptr;
    }

    // Compensation adds nodes and rewrites edges, so the analysis has to be
    // redone afterwards. Once is enough: the delay lines are sized from the
    // final arrival times and report that delay themselves, so the second pass
    // finds every summing point already aligned.
    if (compensate_ && insertDelayCompensation(topology)) {
        topology = analyse();

        if (!topology.acyclic) {
            error_ = "the connections contain a cycle";
            return nullptr;
        }
    }

    const std::size_t nodeCountAfterCompensation = nodes_.size();
    const std::vector<std::vector<NodeIndex>>& sources = topology.sources;
    const std::vector<NodeIndex>&              order   = topology.order;
    const std::vector<FrameCount>&             latencyTo = topology.latencyTo;

    // ── Buffer assignment ─────────────────────────────────────────────────────
    // One buffer per node output. Buffer reuse (handing a node the buffer of a
    // source whose last consumer it is) would cut the working set materially on
    // large graphs, but it is an optimisation, and docs/PERFORMANCE.md §4
    // forbids optimising before measuring. Correct and obvious first.
    auto graph = std::make_unique<CompiledGraph>();

    graph->pool_.allocate(nodeCountAfterCompensation, channelCount, maxBlockSize);
    graph->sampleRate_   = sampleRate;
    graph->maxBlockSize_ = maxBlockSize;
    graph->totalLatency_ = latencyTo[master_];
    graph->masterBuffer_ = master_;
    graph->hasMaster_    = true;
    graph->order_        = order;

    std::size_t widestInput = 0;

    graph->steps_.reserve(nodeCountAfterCompensation);
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
