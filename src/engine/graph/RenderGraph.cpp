#include "engine/graph/RenderGraph.h"

#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/DelayLineNode.h"

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

std::size_t GraphBuilder::insertCompensationDelays(const std::vector<NodeIndex>& order,
                                                   const std::vector<std::vector<NodeIndex>>& sources,
                                                   std::size_t channelCount)
{
    // Longest path to each node's output, walked in topological order so that
    // every source is already known by the time its destination is reached.
    std::vector<FrameCount> latencyTo(nodes_.size(), 0);
    std::size_t             inserted = 0;

    for (const NodeIndex index : order) {
        FrameCount latest = 0;

        for (const NodeIndex source : sources[index])
            latest = std::max(latest, latencyTo[source]);

        // Every source that arrives early is delayed to meet the latest one.
        // Compensating on the *inputs* rather than at the output is what makes
        // this work for a node with several sources: a send and a direct path
        // into the same bus can need different amounts.
        for (const NodeIndex source : sources[index]) {
            const FrameCount deficit = latest - latencyTo[source];
            if (deficit <= 0)
                continue;

            auto delay = std::make_unique<dsp::DelayLineNode>(deficit, channelCount);
            nodes_.push_back(std::move(delay));
            const NodeIndex delayIndex = nodes_.size() - 1;

            // Rewire source -> destination as source -> delay -> destination.
            for (Connection& connection : connections_) {
                if (connection.source == source && connection.destination == index)
                    connection.destination = delayIndex;
            }

            connections_.push_back({delayIndex, index});
            ++inserted;
        }

        latencyTo[index] = latest + nodes_[index]->latencyFrames();
    }

    return inserted;
}

std::unique_ptr<CompiledGraph> GraphBuilder::compile(SampleRate sampleRate, FrameCount maxBlockSize,
                                                     std::size_t channelCount)
{
    error_.clear();
    compensationNodes_ = 0;

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
    //
    // Run up to twice. The first pass establishes the order that delay
    // compensation needs; inserting delay lines changes the graph, so the
    // second pass sorts what will actually be rendered. A third pass is never
    // needed: compensation only ever adds nodes on existing edges, and after
    // one pass every path into every summing point already agrees.
    std::vector<std::vector<NodeIndex>> sources;
    std::vector<NodeIndex>              order;

    for (int pass = 0; pass < 2; ++pass) {
        const std::size_t total = nodes_.size();

        std::vector<std::vector<NodeIndex>> successors(total);
        std::vector<std::size_t>            remainingInputs(total, 0);

        sources.assign(total, {});

        for (const Connection& connection : connections_) {
            successors[connection.source].push_back(connection.destination);
            sources[connection.destination].push_back(connection.source);
            ++remainingInputs[connection.destination];
        }

        std::deque<NodeIndex> ready;

        order.clear();
        order.reserve(total);

        for (NodeIndex index = 0; index < total; ++index)
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

        if (order.size() != total) {
            // Kahn's algorithm stalls exactly when a cycle exists. Rejecting is
            // the only honest outcome: breaking an arbitrary edge would render a
            // graph that is not the one the user built.
            error_ = "the connections contain a cycle";
            return nullptr;
        }

        if (pass == 1 || !compensate_)
            break;

        compensationNodes_ = insertCompensationDelays(order, sources, channelCount);

        if (compensationNodes_ == 0)
            break;   // every path already agreed; the second sort would be identical
    }

    const std::size_t total = nodes_.size();

    // ── Latency accumulation ──────────────────────────────────────────────────
    // Longest path to each node, in frames. After compensation every path into a
    // summing point is equal, so this is also what the device and the recording
    // path must be told the graph costs.
    std::vector<FrameCount> latencyTo(total, 0);

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

    graph->pool_.allocate(total, channelCount, maxBlockSize);
    graph->sampleRate_   = sampleRate;
    graph->maxBlockSize_ = maxBlockSize;
    graph->totalLatency_ = latencyTo[master_];
    graph->masterBuffer_ = master_;
    graph->hasMaster_    = true;
    graph->order_        = order;

    std::size_t widestInput = 0;

    graph->steps_.reserve(total);
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
