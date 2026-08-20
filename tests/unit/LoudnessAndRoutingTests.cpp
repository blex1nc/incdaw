#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/MixerCommands.h"
#include "app/commands/PluginCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/MixerStripNode.h"
#include "engine/dsp/effects/UtilityEffects.h"
#include "engine/transport/TempoMap.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"

#include <atomic>
#include <cmath>
#include <filesystem>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using incdaw::engine::dsp::LoudnessMeterEffect;
using incdaw::engine::dsp::MixerStripNode;

namespace fs = std::filesystem;

namespace {

constexpr FrameCount blockSize = 256;

/// Feeds `effect` a 997 Hz sine of amplitude `amplitude` on the left channel
/// (right silent) for `seconds`, through the node contract.
void feedSine(LoudnessMeterEffect& effect, double sampleRate, double amplitude, double seconds,
              FrameCount& phaseFrames)
{
    AudioBufferPool pool;
    pool.allocate(2, 2, blockSize);

    const AudioBufferView input  = pool.buffer(0);
    const AudioBufferView output = pool.buffer(1);

    const auto frames = static_cast<FrameCount>(sampleRate * seconds);

    for (FrameCount start = 0; start < frames; start += blockSize) {
        const FrameCount count = std::min<FrameCount>(blockSize, frames - start);

        input.clear();
        output.clear();
        for (FrameCount frame = 0; frame < count; ++frame) {
            const double phase = 2.0 * 3.14159265358979323846 * 997.0
                               * static_cast<double>(phaseFrames + frame) / sampleRate;
            input.channel(0)[frame] = static_cast<Sample>(amplitude * std::sin(phase));
        }
        phaseFrames += count;

        ProcessContext context;
        context.output     = output;
        context.inputs     = &input;
        context.inputCount = 1;
        context.frameCount = count;
        context.sampleRate = sampleRate;

        effect.process(context);
    }
}

} // namespace

// ── LUFS ──────────────────────────────────────────────────────────────────────

TEST_CASE("a full-scale 997 Hz left-channel sine reads -3.01 LUFS")
{
    // The BS.1770-4 calibration point, and the test that holds the K-filter
    // design to the spec at two sample rates.
    for (const double sampleRate : { 48000.0, 44100.0 }) {
        LoudnessMeterEffect meter;
        meter.prepare(sampleRate, blockSize);

        FrameCount phase = 0;
        feedSine(meter, sampleRate, 1.0, 5.0, phase);

        CHECK(meter.momentaryLufs() == doctest::Approx(-3.01).epsilon(0.03));
        CHECK(meter.shortTermLufs() == doctest::Approx(-3.01).epsilon(0.03));
        CHECK(meter.integratedLufs() == doctest::Approx(-3.01).epsilon(0.03));
    }
}

TEST_CASE("integration gates out silence")
{
    LoudnessMeterEffect meter;
    meter.prepare(48000.0, blockSize);

    // Two seconds of tone at about -23 LUFS, then eight of silence. An
    // ungated mean would sink several LU; the gate holds the reading.
    FrameCount phase = 0;
    feedSine(meter, 48000.0, 0.1, 2.0, phase);
    const double toneOnly = meter.integratedLufs();
    CHECK(toneOnly == doctest::Approx(-23.01).epsilon(0.03));

    feedSine(meter, 48000.0, 0.0, 8.0, phase);
    CHECK(meter.integratedLufs() == doctest::Approx(toneOnly).epsilon(0.03));

    // The momentary meter, by contrast, has fallen to the floor.
    CHECK(meter.momentaryLufs() < -90.0);
}

TEST_CASE("resetting the integration starts a new programme")
{
    LoudnessMeterEffect meter;
    meter.prepare(48000.0, blockSize);

    FrameCount phase = 0;
    feedSine(meter, 48000.0, 1.0, 2.0, phase);
    CHECK(meter.integratedLufs() == doctest::Approx(-3.01).epsilon(0.03));

    meter.resetIntegration();
    feedSine(meter, 48000.0, 0.1, 2.0, phase);
    CHECK(meter.integratedLufs() == doctest::Approx(-23.01).epsilon(0.03));
}

// ── Stereo separation ─────────────────────────────────────────────────────────

namespace {

/// Runs a strip over a constant hard-left signal until its ramps settle, and
/// returns the last frame of both channels.
std::pair<double, double> stripOutput(MixerStripNode& strip)
{
    AudioBufferPool pool;
    pool.allocate(2, 2, blockSize);

    const AudioBufferView input  = pool.buffer(0);
    const AudioBufferView output = pool.buffer(1);

    double left = 0.0, right = 0.0;
    for (int block = 0; block < 40; ++block) {   // ~0.2 s: every ramp settles
        input.clear();
        output.clear();
        for (FrameCount frame = 0; frame < blockSize; ++frame)
            input.channel(0)[frame] = Sample{1};

        ProcessContext context;
        context.output     = output;
        context.inputs     = &input;
        context.inputCount = 1;
        context.frameCount = blockSize;
        context.sampleRate = 48000.0;

        strip.process(context);
        left  = static_cast<double>(output.channel(0)[blockSize - 1]);
        right = static_cast<double>(output.channel(1)[blockSize - 1]);
    }
    return { left, right };
}

} // namespace

TEST_CASE("stereo separation collapses to mono at -1 and widens at +1")
{
    // A hard-left source: mid and side are both half. Centre pan scales each
    // output channel by 1/sqrt(2).
    const double centre = 1.0 / std::sqrt(2.0);

    MixerStripNode neutral;
    neutral.prepare(48000.0, blockSize);
    const auto untouched = stripOutput(neutral);
    CHECK(untouched.first == doctest::Approx(centre).epsilon(1e-3));
    CHECK(untouched.second == doctest::Approx(0.0).epsilon(1e-3));

    MixerStripNode mono;
    mono.prepare(48000.0, blockSize);
    mono.setStereoSeparation(-1.0);
    const auto collapsed = stripOutput(mono);
    CHECK(collapsed.first == doctest::Approx(0.5 * centre).epsilon(1e-3));
    CHECK(collapsed.second == doctest::Approx(0.5 * centre).epsilon(1e-3));

    MixerStripNode wide;
    wide.prepare(48000.0, blockSize);
    wide.setStereoSeparation(1.0);
    const auto widened = stripOutput(wide);
    CHECK(widened.first == doctest::Approx(1.5 * centre).epsilon(1e-3));
    CHECK(widened.second == doctest::Approx(-0.5 * centre).epsilon(1e-3));
}

TEST_CASE("stereo separation is a mergeable command and survives save/load")
{
    project::Project project;
    app::CommandRegistry registry { project };

    auto add = std::make_unique<app::AddMixerNodeCommand>(project::MixerNodeType::track, "Wide");
    auto* raw = add.get();
    REQUIRE(registry.execute(std::move(add)));
    const project::EntityId node = raw->mixerNodeId();

    REQUIRE(registry.executeMerging(
        std::make_unique<app::SetMixerStereoSeparationCommand>(node, 0.25)));
    REQUIRE(registry.executeMerging(
        std::make_unique<app::SetMixerStereoSeparationCommand>(node, 0.5)));

    CHECK(project.findMixerNode(node)->stereoSeparation == doctest::Approx(0.5));
    CHECK(registry.undoDepth() == 2);   // the add, then one merged gesture

    CHECK(registry.undo());
    CHECK(project.findMixerNode(node)->stereoSeparation == doctest::Approx(0.0));
    CHECK(registry.redo());

    const fs::path file =
        fs::temp_directory_path() / "incdaw-test-separation" / "Separation.incdaw";
    std::error_code code;
    fs::create_directories(file.parent_path(), code);

    REQUIRE(project::ProjectFile::save(project, file));

    project::Project loaded;
    REQUIRE(project::ProjectFile::load(loaded, file));
    CHECK(loaded.findMixerNode(node)->stereoSeparation == doctest::Approx(0.5));

    fs::remove_all(file.parent_path(), code);
}

// ── Pre-fader sends ───────────────────────────────────────────────────────────

namespace {

class ConstantSourceInsert final : public Node {
public:
    explicit ConstantSourceInsert(double level) : level_(level) {}

    void process(const ProcessContext& context) noexcept override
    {
        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            Sample* samples = context.output.channel(channel);
            for (FrameCount frame = 0; frame < context.frameCount; ++frame)
                samples[frame] = static_cast<Sample>(level_);
        }
    }

    [[nodiscard]] const char* name() const noexcept override { return "ConstantSource"; }

private:
    double level_ = 0.0;
};

/// One strip with a constant source and its fader pulled to zero; the send
/// decides whether anything reaches the master.
struct SendFixture {
    project::Project project;
    app::CommandRegistry registry { project };
    engine::TempoMap tempo;
    project::EntityId strip;

    SendFixture()
    {
        tempo.setSampleRate(48000.0);

        auto add = std::make_unique<app::AddMixerNodeCommand>(
            project::MixerNodeType::track, "Source");
        auto* raw = add.get();
        REQUIRE(registry.execute(std::move(add)));
        strip = raw->mixerNodeId();

        REQUIRE(registry.execute(std::make_unique<app::AddInsertCommand>(
            strip, plugins::PluginIdentifier{ plugins::Format::clap, "test.source" })));
        REQUIRE(registry.execute(std::make_unique<app::SetMixerVolumeCommand>(strip, 0.0)));
    }

    double masterLevel(bool preFader)
    {
        REQUIRE(registry.execute(std::make_unique<app::ConnectMixerCommand>(
            strip, project.masterMixerNode(), true, 1.0, preFader)));

        project::GraphCompileOptions options;
        options.masterGain    = engine::Sample{1.0f};
        options.insertFactory = [](const project::PluginSlot& slot, std::string& error)
            -> std::unique_ptr<engine::Node> {
            if (slot.plugin.uid == "test.source")
                return std::make_unique<ConstantSourceInsert>(0.1);
            error = "unknown test insert";
            return nullptr;
        };

        auto compiled = project::compileProjectGraph(project, tempo, options);
        REQUIRE(compiled);

        AudioBufferPool pool;
        pool.allocate(1, 2, blockSize);
        const AudioBufferView view = pool.buffer(0);

        double last = 0.0;
        for (int block = 0; block < 40; ++block) {
            compiled.graph->process(view, blockSize,
                                    static_cast<FramePosition>(block) * blockSize);
            last = std::fabs(static_cast<double>(view.channel(0)[blockSize - 1]));
        }

        // Remove the send so the next scenario starts clean.
        REQUIRE(registry.undo());
        return last;
    }
};

} // namespace

TEST_CASE("a pre-fader send survives the fader; a post-fader send dies with it")
{
    SendFixture fixture;

    // Pre-fader: the tap sits before the zeroed fader. 0.1 crosses only the
    // master's centre pan: 0.1 / sqrt(2).
    const double pre = fixture.masterLevel(true);
    CHECK(pre == doctest::Approx(0.1 / std::sqrt(2.0)).epsilon(1e-3));

    const double post = fixture.masterLevel(false);
    CHECK(post == doctest::Approx(0.0).epsilon(1e-6));
}
