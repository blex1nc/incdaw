#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/GainNode.h"
#include "engine/dsp/SineOscillatorNode.h"
#include "engine/graph/RenderGraph.h"

#include <cmath>
#include <memory>

using namespace incdaw::engine;

namespace {

/// Writes a constant into every channel — makes summing and routing observable.
class ConstantNode final : public Node {
public:
    explicit ConstantNode(Sample value, FrameCount latency = 0) noexcept
        : value_(value), latency_(latency) {}

    void process(const ProcessContext& context) noexcept override
    {
        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            Sample* samples = context.output.channel(channel);
            for (FrameCount frame = 0; frame < context.frameCount; ++frame)
                samples[frame] = value_;
        }
    }

    [[nodiscard]] FrameCount latencyFrames() const noexcept override { return latency_; }
    [[nodiscard]] const char* name() const noexcept override { return "Constant"; }

private:
    Sample     value_;
    FrameCount latency_;
};

/// Records the order in which nodes ran, to verify the topological sort.
class OrderRecordingNode final : public Node {
public:
    OrderRecordingNode(int identifier, std::vector<int>& log) noexcept
        : identifier_(identifier), log_(&log) {}

    void process(const ProcessContext&) noexcept override { log_->push_back(identifier_); }

private:
    int               identifier_;
    std::vector<int>* log_;
};

/// A device-style destination buffer for tests.
struct TestOutput {
    explicit TestOutput(std::size_t channels, FrameCount frames)
    {
        pool.allocate(1, channels, frames);
    }

    [[nodiscard]] AudioBufferView view() const noexcept { return pool.buffer(0); }

    AudioBufferPool pool;
};

} // namespace

TEST_CASE("a graph with no master does not compile")
{
    GraphBuilder builder;
    builder.addNode(std::make_unique<ConstantNode>(Sample{1}));

    CHECK(builder.compile(48000.0, 128, 2) == nullptr);
    CHECK(builder.lastError() == "no valid master node");
}

TEST_CASE("an empty graph does not compile")
{
    GraphBuilder builder;
    CHECK(builder.compile(48000.0, 128, 2) == nullptr);
    CHECK(builder.lastError() == "graph has no nodes");
}

TEST_CASE("invalid formats are rejected rather than producing an unusable graph")
{
    for (int variant = 0; variant < 3; ++variant) {
        GraphBuilder builder;
        const auto node = builder.addNode(std::make_unique<ConstantNode>(Sample{1}));
        builder.setMaster(node);

        const auto graph = builder.compile(variant == 0 ? 0.0 : 48000.0,
                                           variant == 1 ? 0 : 128,
                                           variant == 2 ? 0u : 2u);
        CHECK(graph == nullptr);
        CHECK_FALSE(builder.lastError().empty());
    }
}

TEST_CASE("a connection to a node that does not exist is rejected")
{
    GraphBuilder builder;
    const auto node = builder.addNode(std::make_unique<ConstantNode>(Sample{1}));
    builder.connect(node, 99);
    builder.setMaster(node);

    CHECK(builder.compile(48000.0, 128, 2) == nullptr);
    CHECK(builder.lastError() == "connection references a node that does not exist");
}

TEST_CASE("a cycle is rejected, not silently broken")
{
    // Breaking an arbitrary edge would render a graph that is not the one the
    // user built, and the difference would be inaudible until it mattered.
    GraphBuilder builder;

    const auto first  = builder.addNode(std::make_unique<ConstantNode>(Sample{1}));
    const auto second = builder.addNode(std::make_unique<dsp::GainNode>());
    const auto third  = builder.addNode(std::make_unique<dsp::GainNode>());

    builder.connect(first, second);
    builder.connect(second, third);
    builder.connect(third, second);   // closes the loop
    builder.setMaster(third);

    CHECK(builder.compile(48000.0, 128, 2) == nullptr);
    CHECK(builder.lastError() == "the connections contain a cycle");
}

TEST_CASE("a node connected to itself is rejected")
{
    GraphBuilder builder;
    const auto node = builder.addNode(std::make_unique<dsp::GainNode>());
    builder.connect(node, node);
    builder.setMaster(node);

    CHECK(builder.compile(48000.0, 128, 2) == nullptr);
    CHECK(builder.lastError() == "a node cannot be connected to itself");
}

TEST_CASE("nodes run in dependency order")
{
    std::vector<int> executionLog;

    GraphBuilder builder;
    const auto a = builder.addNode(std::make_unique<OrderRecordingNode>(1, executionLog));
    const auto b = builder.addNode(std::make_unique<OrderRecordingNode>(2, executionLog));
    const auto c = builder.addNode(std::make_unique<OrderRecordingNode>(3, executionLog));

    // c depends on b, b depends on a — declared out of order on purpose.
    builder.connect(b, c);
    builder.connect(a, b);
    builder.setMaster(c);

    const auto graph = builder.compile(48000.0, 64, 2);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 64};
    graph->process(output.view(), 64, 0);

    REQUIRE(executionLog.size() == 3);
    CHECK(executionLog[0] == 1);
    CHECK(executionLog[1] == 2);
    CHECK(executionLog[2] == 3);
}

TEST_CASE("a node with several sources receives their sum")
{
    GraphBuilder builder;

    const auto first  = builder.addNode(std::make_unique<ConstantNode>(Sample{0.25f}));
    const auto second = builder.addNode(std::make_unique<ConstantNode>(Sample{0.5f}));
    const auto mix    = builder.addNode(std::make_unique<dsp::GainNode>(Sample{1}));

    builder.connect(first, mix);
    builder.connect(second, mix);
    builder.setMaster(mix);

    const auto graph = builder.compile(48000.0, 32, 2);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 32};
    graph->process(output.view(), 32, 0);

    CHECK(output.view().channel(0)[0] == doctest::Approx(0.75f));
    CHECK(output.view().channel(1)[31] == doctest::Approx(0.75f));
}

TEST_CASE("latency accumulates along the longest path to the master")
{
    GraphBuilder builder;

    const auto source = builder.addNode(std::make_unique<ConstantNode>(Sample{1}, 100));
    const auto shortPath = builder.addNode(std::make_unique<ConstantNode>(Sample{0}, 10));
    const auto longPath  = builder.addNode(std::make_unique<ConstantNode>(Sample{0}, 250));
    const auto master    = builder.addNode(std::make_unique<dsp::GainNode>());

    builder.connect(source, shortPath);
    builder.connect(source, longPath);
    builder.connect(shortPath, master);
    builder.connect(longPath, master);
    builder.setMaster(master);

    const auto graph = builder.compile(48000.0, 128, 2);
    REQUIRE(graph != nullptr);

    // 100 (source) + 250 (longer branch); the 10-frame branch does not reduce it.
    CHECK(graph->latencyFrames() == 350);
}

TEST_CASE("a node that writes nothing leaves silence, not the previous block")
{
    // Buffers are reused every block. Without an explicit clear, a node that
    // returns early would replay whatever it rendered last time.
    class SometimesSilentNode final : public Node {
    public:
        void process(const ProcessContext& context) noexcept override
        {
            if (!writeThisBlock)
                return;

            for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel)
                for (FrameCount frame = 0; frame < context.frameCount; ++frame)
                    context.output.channel(channel)[frame] = Sample{1};
        }

        bool writeThisBlock = true;
    };

    auto owned = std::make_unique<SometimesSilentNode>();
    auto* node = owned.get();

    GraphBuilder builder;
    const auto index = builder.addNode(std::move(owned));
    builder.setMaster(index);

    const auto graph = builder.compile(48000.0, 16, 1);
    REQUIRE(graph != nullptr);

    TestOutput output{1, 16};

    graph->process(output.view(), 16, 0);
    CHECK(output.view().peak() == doctest::Approx(1.0f));

    node->writeThisBlock = false;
    graph->process(output.view(), 16, 0);
    CHECK(output.view().peak() == doctest::Approx(0.0f));
}

TEST_CASE("rendering a graph allocates nothing")
{
    // The Phase 2 exit criterion, expressed as a test.
    GraphBuilder builder;

    const auto oscillator = builder.addNode(std::make_unique<dsp::SineOscillatorNode>(440.0));
    const auto gain       = builder.addNode(std::make_unique<dsp::GainNode>(Sample{0.5f}));

    builder.connect(oscillator, gain);
    builder.setMaster(gain);

    const auto graph = builder.compile(48000.0, 128, 2);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 128};

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 500; ++block)
            graph->process(output.view(), 128, block * 128);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

TEST_CASE("a block shorter than the compiled maximum renders correctly")
{
    // Devices are entitled to hand over fewer frames than the configured size.
    GraphBuilder builder;
    const auto node = builder.addNode(std::make_unique<ConstantNode>(Sample{1}));
    builder.setMaster(node);

    const auto graph = builder.compile(48000.0, 512, 2);
    REQUIRE(graph != nullptr);

    TestOutput output{2, 512};
    output.view().clear();
    graph->process(output.view().subBlock(0, 64), 64, 0);

    CHECK(output.view().channel(0)[0] == doctest::Approx(1.0f));
    CHECK(output.view().channel(0)[63] == doctest::Approx(1.0f));
    CHECK(output.view().channel(0)[64] == doctest::Approx(0.0f));   // untouched
}

TEST_CASE("the builder is emptied by compiling, so it cannot be reused by accident")
{
    GraphBuilder builder;
    const auto node = builder.addNode(std::make_unique<ConstantNode>(Sample{1}));
    builder.setMaster(node);

    REQUIRE(builder.compile(48000.0, 64, 2) != nullptr);

    CHECK(builder.nodeCount() == 0);
    CHECK(builder.compile(48000.0, 64, 2) == nullptr);
}
