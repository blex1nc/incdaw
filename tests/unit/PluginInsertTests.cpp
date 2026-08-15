// Phase 13 — insert chains in the compiled project graph.
//
// Two halves, deliberately. The first uses synthetic insert nodes: the
// compiler's contract is `engine::Node`, so the properties that matter —
// chain order, position relative to the fader, bypass, one node per slot,
// a slot that cannot be built — are provable without a plugin binary in
// sight. That is the whole point of the injected factory (D-028).
//
// The second half is the end-to-end one: a real hosted CLAP, resolved
// through the registry and the instance manager, inside the graph the
// project compiles.

#include "doctest.h"

#include "engine/core/AudioBufferPool.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/graph/Node.h"
#include "engine/transport/TempoMap.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
namespace fs = std::filesystem;

namespace {

using engine::FrameCount;
using engine::FramePosition;
using engine::Sample;

constexpr FrameCount blockSize   = 256;
constexpr int        blockCount  = 8;

/// The insert nodes below all sum their graph inputs first, exactly as
/// plugins::PluginNode does: an insert processes what reached the strip, not
/// one source at a time.
class SummingInsert : public engine::Node {
public:
    void process(const engine::ProcessContext& context) noexcept override
    {
        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            Sample* out = context.output.channel(channel);

            for (std::size_t input = 0; input < context.inputCount; ++input) {
                const auto view = context.input(input);
                if (channel >= view.channelCount())
                    continue;

                const Sample* source = view.channel(channel);
                for (FrameCount frame = 0; frame < context.frameCount; ++frame)
                    out[frame] += source[frame];
            }

            shape(out, context.frameCount);
        }
    }

protected:
    virtual void shape(Sample* samples, FrameCount frameCount) noexcept = 0;
};

class GainInsert final : public SummingInsert {
public:
    explicit GainInsert(Sample factor) noexcept : factor_(factor) {}

    [[nodiscard]] const char* name() const noexcept override { return "TestGainInsert"; }

private:
    void shape(Sample* samples, FrameCount frameCount) noexcept override
    {
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            samples[frame] *= factor_;
    }

    Sample factor_;
};

/// Non-linear on purpose: gain multiplications commute, so a chain built out
/// of them could not tell a pre-fader insert from a post-fader one.
class ClipInsert final : public SummingInsert {
public:
    explicit ClipInsert(Sample threshold) noexcept : threshold_(threshold) {}

    [[nodiscard]] const char* name() const noexcept override { return "TestClipInsert"; }

private:
    void shape(Sample* samples, FrameCount frameCount) noexcept override
    {
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            samples[frame] = std::clamp(samples[frame], -threshold_, threshold_);
    }

    Sample threshold_;
};

/// A project with one audible channel playing one note, so that compiling it
/// produces a signal to put an insert into.
struct InsertFixture {
    project::Project  project;
    project::EntityId channel;
    project::EntityId pattern;
    engine::TempoMap  tempo;

    InsertFixture()
    {
        channel = project.addChannel("Channel 1").id;

        // Loud on purpose: a clipping test whose signal never reaches the
        // threshold proves nothing, and the assertions below check that it
        // does reach it rather than assuming.
        project.channels().front().volume = 4.0;

        pattern = project.addPattern("Pattern 1").id;

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = 0;
        note.duration = engine::ticksPerQuarterNote * 4;
        note.key      = 60;
        note.value    = 100;
        project.findPattern(pattern)->contentFor(channel).events.push_back(note);

        tempo.setSampleRate(48000.0);
    }

    project::MixerNode& master()
    {
        for (project::MixerNode& node : project.mixerNodes())
            if (node.id == project.masterMixerNode())
                return node;

        FAIL("the project has no master mixer node");
        return project.mixerNodes().front();
    }

    /// Appends an insert slot to the master, and returns it for editing.
    project::PluginSlot& addMasterInsert(std::string uid)
    {
        project::PluginSlot slot;
        slot.id            = project.ids().next();
        slot.plugin.format = plugins::Format::clap;
        slot.plugin.uid    = std::move(uid);

        master().inserts.push_back(slot);
        return master().inserts.back();
    }

    [[nodiscard]] project::CompiledProjectGraph compile(project::InsertFactory factory = {})
    {
        project::GraphCompileOptions options;
        options.sampleRate    = 48000.0;
        options.maxBlockSize  = blockSize;
        options.pattern       = pattern;
        options.masterGain    = Sample{1};   // no hidden headroom in the arithmetic
        options.insertFactory = std::move(factory);

        return project::compileProjectGraph(project, tempo, options);
    }
};

/// Renders the graph and returns the left channel, blocks concatenated.
std::vector<Sample> render(engine::CompiledGraph& graph)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<Sample> samples;
    samples.reserve(static_cast<std::size_t>(blockSize) * blockCount);

    for (int block = 0; block < blockCount; ++block) {
        pool.buffer(0).clear();
        graph.process(pool.buffer(0), blockSize,
                      static_cast<FramePosition>(block) * blockSize);

        const Sample* left = pool.buffer(0).channel(0);
        samples.insert(samples.end(), left, left + blockSize);
    }

    return samples;
}

/// Renders with the realtime guard armed around the audio work only: the
/// pool and the result vector are allocated first, because a test that
/// allocates its own scratch inside the scope measures itself.
std::vector<Sample> renderUnderRealtimeGuard(engine::CompiledGraph& graph)
{
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);

    std::vector<Sample> samples(static_cast<std::size_t>(blockSize) * blockCount, Sample{0});

    for (int block = 0; block < blockCount; ++block) {
        pool.buffer(0).clear();

        {
            const engine::rt::ScopedRealtimeContext realtimeScope;
            graph.process(pool.buffer(0), blockSize,
                          static_cast<FramePosition>(block) * blockSize);
        }

        const Sample* left = pool.buffer(0).channel(0);
        std::copy(left, left + blockSize,
                  samples.begin() + static_cast<std::ptrdiff_t>(block) * blockSize);
    }

    return samples;
}

[[nodiscard]] bool anyNonZero(const std::vector<Sample>& samples)
{
    return std::any_of(samples.begin(), samples.end(),
                       [](Sample value) { return value != Sample{0}; });
}

/// A factory that hands out fixed nodes in order, and counts how many were
/// asked for. The counter is what proves "one node per slot".
struct ScriptedFactory {
    std::vector<std::function<std::unique_ptr<engine::Node>()>> makers;
    int    requests = 0;
    bool   fail     = false;

    project::InsertFactory operator()()
    {
        return [this](const project::PluginSlot&, std::string& error)
                   -> std::unique_ptr<engine::Node> {
            const auto index = static_cast<std::size_t>(requests++);

            if (fail || index >= makers.size()) {
                error = "no such plugin";
                return nullptr;
            }

            return makers[index]();
        };
    }
};

} // namespace

// ── Synthetic inserts: the compiler's own contract ────────────────────────────

TEST_CASE("an insert sits in the signal path of the strip that owns it")
{
    InsertFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);
    REQUIRE(anyNonZero(dry));

    fixture.addMasterInsert("test.gain");

    ScriptedFactory factory;
    factory.makers.push_back([] { return std::make_unique<GainInsert>(Sample{0.5}); });

    auto compiled = fixture.compile(factory());
    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    CHECK(factory.requests == 1);

    const auto wet = render(*compiled.graph);
    REQUIRE(wet.size() == dry.size());

    // Exactly half: 0.5 is exact in binary, so this is an equality test and
    // not an approximation. A strip whose gain smoothing had not settled
    // would break it, which is the point — prepare() snaps for this reason.
    for (std::size_t index = 0; index < wet.size(); ++index)
        REQUIRE(wet[index] == dry[index] * Sample{0.5});
}

TEST_CASE("inserts chain in the order they are listed")
{
    // [clip, gain] clips first and then amplifies; [gain, clip] amplifies
    // into the clipper. Same two processors, different sound — which is what
    // makes insert order a user-visible property rather than an internal
    // detail.
    const auto renderChain = [](bool clipFirst) {
        InsertFixture fixture;
        fixture.addMasterInsert("a");
        fixture.addMasterInsert("b");

        ScriptedFactory factory;
        if (clipFirst) {
            factory.makers.push_back([] { return std::make_unique<ClipInsert>(Sample{0.25}); });
            factory.makers.push_back([] { return std::make_unique<GainInsert>(Sample{2}); });
        } else {
            factory.makers.push_back([] { return std::make_unique<GainInsert>(Sample{2}); });
            factory.makers.push_back([] { return std::make_unique<ClipInsert>(Sample{0.25}); });
        }

        auto compiled = fixture.compile(factory());
        REQUIRE(compiled);
        REQUIRE(factory.requests == 2);

        return render(*compiled.graph);
    };

    const auto clipThenGain = renderChain(true);
    const auto gainThenClip = renderChain(false);

    REQUIRE(anyNonZero(clipThenGain));
    REQUIRE(clipThenGain.size() == gainThenClip.size());

    CHECK(clipThenGain != gainThenClip);

    // And in the direction the arithmetic predicts: clipping last can never
    // exceed the threshold through the strip's own gain, clipping first can.
    const auto peak = [](const std::vector<Sample>& samples) {
        Sample highest = 0;
        for (Sample value : samples)
            highest = std::max(highest, std::abs(value));
        return highest;
    };

    CHECK(peak(clipThenGain) > peak(gainThenClip));
}

TEST_CASE("inserts run before the fader")
{
    InsertFixture fixture;

    // The fader at half, so that pre- and post-fader placement of a clipper
    // give measurably different answers.
    fixture.master().volume = 0.5;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto faded = render(*baseline.graph);
    REQUIRE(anyNonZero(faded));

    // What the strip multiplies by: the fader and the pan law together. Taken
    // from the same function the audio thread uses rather than restated here.
    Sample panLeft  = 0;
    Sample panRight = 0;
    engine::dsp::MixerStripNode::panGains(0.0, panLeft, panRight);

    const auto stripGain = static_cast<Sample>(0.5) * panLeft;
    REQUIRE(stripGain > Sample{0});

    constexpr Sample threshold{0.25};

    fixture.addMasterInsert("clip");

    ScriptedFactory factory;
    factory.makers.push_back([threshold] { return std::make_unique<ClipInsert>(threshold); });

    auto compiled = fixture.compile(factory());
    REQUIRE(compiled);

    const auto clipped = render(*compiled.graph);
    REQUIRE(clipped.size() == faded.size());

    bool everClipped = false;

    for (std::size_t index = 0; index < clipped.size(); ++index) {
        // Pre-fader: the clipper saw the signal before the strip touched it.
        const Sample beforeStrip = faded[index] / stripGain;
        const Sample expected    = std::clamp(beforeStrip, -threshold, threshold) * stripGain;

        REQUIRE(clipped[index] == doctest::Approx(expected));

        if (std::abs(beforeStrip) > threshold)
            everClipped = true;
    }

    // Without this the test would pass on a signal the clipper never touched.
    CHECK(everClipped);
}

TEST_CASE("a bypassed slot is absent from the graph, not a multiply by one")
{
    InsertFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);

    fixture.addMasterInsert("test.gain").bypassed = true;

    ScriptedFactory factory;
    factory.makers.push_back([] { return std::make_unique<GainInsert>(Sample{0.5}); });

    auto compiled = fixture.compile(factory());
    REQUIRE(compiled);

    // The factory was never asked: bypass costs nothing at all, and in
    // particular does not instantiate a plugin.
    CHECK(factory.requests == 0);
    CHECK(compiled.warnings.empty());
    CHECK(render(*compiled.graph) == dry);
}

TEST_CASE("a slot that cannot be built is a pass-through with a reason")
{
    InsertFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);

    fixture.addMasterInsert("com.missing.reverb");

    ScriptedFactory factory;
    factory.fail = true;

    auto compiled = fixture.compile(factory());

    // The project still plays. A missing plugin costs its own slot, never the
    // rest of the mix.
    REQUIRE(compiled);
    CHECK(render(*compiled.graph) == dry);

    REQUIRE(compiled.warnings.size() == 1);
    CHECK(compiled.warnings.front().find("com.missing.reverb") != std::string::npos);
    CHECK(compiled.warnings.front().find("no such plugin") != std::string::npos);
}

TEST_CASE("a build with no plugin host says so rather than dropping the insert silently")
{
    InsertFixture fixture;
    fixture.addMasterInsert("com.acme.reverb");

    auto compiled = fixture.compile();   // no factory at all
    REQUIRE(compiled);

    REQUIRE(compiled.warnings.size() == 1);
    CHECK(compiled.warnings.front().find("com.acme.reverb") != std::string::npos);
}

TEST_CASE("one insert node per slot, however many sources reach the strip")
{
    InsertFixture fixture;

    // A second channel into the same master. An insert wired per incoming
    // edge would process each source separately and be instantiated twice.
    fixture.project.addChannel("Channel 2");

    fixture.addMasterInsert("test.gain");

    ScriptedFactory factory;
    factory.makers.push_back([] { return std::make_unique<GainInsert>(Sample{0.5}); });

    auto compiled = fixture.compile(factory());
    REQUIRE(compiled);
    CHECK(factory.requests == 1);
}

// ── A real hosted plugin, end to end ─────────────────────────────────────────

namespace {

struct ScratchDir {
    fs::path path;

    explicit ScratchDir(const char* name) : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDir()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }
};

} // namespace

TEST_CASE("EXIT CRITERION: a hosted plugin processes as a mixer insert")
{
    ScratchDir folder{"incdaw-insert-plugins"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, folder.path / "gain.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 1);
    REQUIRE(registry.find("com.incdaw.testgain").library != nullptr);

    plugins::PluginInstanceManager manager{registry};

    InsertFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);
    const auto dry = render(*baseline.graph);
    REQUIRE(anyNonZero(dry));

    fixture.addMasterInsert("com.incdaw.testgain");

    const auto factory = [&manager](const project::PluginSlot& slot, std::string& error) {
        return manager.createInsert(slot.plugin, 48000.0, blockSize, error);
    };

    auto compiled = fixture.compile(factory);
    REQUIRE(compiled);
    CHECK(compiled.warnings.empty());
    CHECK(manager.loadedLibraryCount() == 1);

    // Rendered under the realtime guard: hosting a plugin must not make the
    // audio thread allocate, whatever the plugin does on its own time.
    engine::rt::resetViolations();

    const auto wet = renderUnderRealtimeGuard(*compiled.graph);

    REQUIRE(wet.size() == dry.size());

    // The test plugin is a fixed -6 dB.
    for (std::size_t index = 0; index < wet.size(); ++index)
        REQUIRE(wet[index] == dry[index] * Sample{0.5});

    if (engine::rt::guardEnabled()) {
        CHECK(engine::rt::allocationViolations() == 0);
        CHECK(engine::rt::deallocationViolations() == 0);
    }
}

TEST_CASE("the instance manager reuses one library and names what it cannot find")
{
    ScratchDir folder{"incdaw-insert-manager"};
    fs::copy_file(INCDAW_TESTGAIN_PLUGIN, folder.path / "gain.clap");

    plugins::PluginRegistry registry;
    REQUIRE(registry.scanDirectory(folder.path, INCDAW_PLUGINSCAN_BINARY) == 1);

    plugins::PluginInstanceManager manager{registry};

    plugins::PluginIdentifier identifier;
    identifier.format = plugins::Format::clap;
    identifier.uid    = "com.incdaw.testgain";

    std::string error;
    auto first = manager.createInsert(identifier, 48000.0, blockSize, error);
    REQUIRE(first != nullptr);

    auto second = manager.createInsert(identifier, 48000.0, blockSize, error);
    REQUIRE(second != nullptr);

    // Two instances, one library: the binary is opened once and outlives every
    // graph built from it.
    CHECK(manager.loadedLibraryCount() == 1);

    identifier.uid = "com.nobody.nothing";
    CHECK(manager.createInsert(identifier, 48000.0, blockSize, error) == nullptr);
    CHECK(error.find("com.nobody.nothing") != std::string::npos);

    identifier.format = plugins::Format::vst3;
    identifier.uid    = "whatever";
    CHECK(manager.createInsert(identifier, 48000.0, blockSize, error) == nullptr);
    CHECK(error.find("vst3") != std::string::npos);
}
