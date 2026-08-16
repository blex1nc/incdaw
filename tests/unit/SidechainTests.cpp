#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/MixerCommands.h"
#include "app/commands/PluginCommands.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/effects/DynamicsEffects.h"
#include "engine/transport/TempoMap.h"
#include "project/ProjectGraphCompiler.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using incdaw::engine::dsp::CompressorEffect;

namespace {

constexpr FrameCount blockSize  = 256;
constexpr double     sampleRate = 48000.0;

/// Runs the compressor over constant main and key signals for `blocks`
/// blocks, returning the last block's output amplitude on channel 0.
double runKeyed(CompressorEffect& compressor, double mainLevel, double keyLevel, int blocks,
                bool wireKey = true)
{
    AudioBufferPool pool;
    pool.allocate(3, 2, blockSize);

    const AudioBufferView inputs[2] = { pool.buffer(0), pool.buffer(1) };
    const AudioBufferView output    = pool.buffer(2);

    double last = 0.0;

    for (int block = 0; block < blocks; ++block) {
        for (std::size_t channel = 0; channel < 2; ++channel) {
            for (FrameCount frame = 0; frame < blockSize; ++frame) {
                inputs[0].channel(channel)[frame] = static_cast<Sample>(mainLevel);
                inputs[1].channel(channel)[frame] = static_cast<Sample>(keyLevel);
            }
        }
        output.clear();

        ProcessContext context;
        context.output     = output;
        context.inputs     = inputs;
        context.inputCount = wireKey ? 2 : 1;
        context.frameCount = blockSize;
        context.sampleRate = sampleRate;

        compressor.process(context);
        last = std::fabs(static_cast<double>(output.channel(0)[blockSize - 1]));
    }

    return last;
}

} // namespace

// ── Node level ────────────────────────────────────────────────────────────────

TEST_CASE("an external key ducks the main signal")
{
    CompressorEffect compressor;
    compressor.prepare(sampleRate, blockSize);
    compressor.setKeyInput(1);

    compressor.setParameter(CompressorEffect::thresholdDb, -20.0);
    compressor.setParameter(CompressorEffect::ratio, 20.0);
    compressor.setParameter(CompressorEffect::attackMs, 0.1);
    compressor.setParameter(CompressorEffect::releaseMs, 50.0);

    // Main sits exactly at threshold; the loud key drives the reduction.
    const double ducked = runKeyed(compressor, 0.1, 1.0, 20);
    CHECK(ducked < 0.03);

    // Key gone: the main signal alone never crosses the threshold hard enough
    // to stay squashed, so the gain releases back to (nearly) unity.
    const double released = runKeyed(compressor, 0.1, 0.0, 40);
    CHECK(released > 0.09);
}

TEST_CASE("the key feeds the detector but never the audio path")
{
    CompressorEffect compressor;
    compressor.prepare(sampleRate, blockSize);
    compressor.setKeyInput(1);

    // Ratio 1 disengages the gain computer: the output must be the main
    // input untouched, however loud the key.
    compressor.setParameter(CompressorEffect::ratio, 1.0);

    const double out = runKeyed(compressor, 0.25, 1.0, 3);
    CHECK(out == doctest::Approx(0.25).epsilon(1.0e-9));
}

TEST_CASE("without a key assignment a second input sums, as any insert's does")
{
    CompressorEffect compressor;
    compressor.prepare(sampleRate, blockSize);
    compressor.setParameter(CompressorEffect::ratio, 1.0);

    const double out = runKeyed(compressor, 0.25, 0.5, 1);
    CHECK(out == doctest::Approx(0.75).epsilon(1.0e-6));
}

// ── Compiled project level ────────────────────────────────────────────────────

namespace {

/// An insert that ignores its inputs and emits a constant — a deterministic
/// signal source that lives where the compiler can build one: a plugin slot.
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

/// Two mixer strips driven by constant test sources: a quiet "bass" strip
/// with a compressor, and a loud "kick" strip that reaches the mix only as a
/// sidechain key. No instruments, no envelopes — every level is exact.
struct SidechainFixture {
    project::Project project;
    app::CommandRegistry registry { project };
    engine::TempoMap tempo;

    project::EntityId bassStrip;
    project::EntityId kickStrip;
    project::EntityId compressorSlot;

    SidechainFixture()
    {
        tempo.setSampleRate(sampleRate);

        auto addBass = std::make_unique<app::AddMixerNodeCommand>(
            project::MixerNodeType::track, "Bass");
        auto* rawBass = addBass.get();
        REQUIRE(registry.execute(std::move(addBass)));
        bassStrip = rawBass->mixerNodeId();

        auto addKick = std::make_unique<app::AddMixerNodeCommand>(
            project::MixerNodeType::track, "Kick");
        auto* rawKick = addKick.get();
        REQUIRE(registry.execute(std::move(addKick)));
        kickStrip = rawKick->mixerNodeId();

        // Sources are inserts built by the factory below.
        REQUIRE(registry.execute(std::make_unique<app::AddInsertCommand>(
            bassStrip,
            plugins::PluginIdentifier{ plugins::Format::clap, "test.source.quiet" })));
        REQUIRE(registry.execute(std::make_unique<app::AddInsertCommand>(
            kickStrip,
            plugins::PluginIdentifier{ plugins::Format::clap, "test.source.loud" })));

        auto insert = std::make_unique<app::AddInsertCommand>(
            bassStrip, plugins::PluginIdentifier{ plugins::Format::builtin,
                                                  "incdaw.compressor" });
        auto* rawInsert = insert.get();
        REQUIRE(registry.execute(std::move(insert)));
        compressorSlot = rawInsert->slotId();

        // Only the bass reaches the master; the kick exists purely as a key.
        REQUIRE(registry.execute(std::make_unique<app::ConnectMixerCommand>(
            bassStrip, project.masterMixerNode())));
    }

    project::CompiledProjectGraph compile()
    {
        project::GraphCompileOptions options;
        options.masterGain = engine::Sample{1.0f};
        options.insertFactory = [](const project::PluginSlot& slot, std::string& error)
            -> std::unique_ptr<engine::Node> {
            if (slot.plugin.uid == "test.source.quiet")
                return std::make_unique<ConstantSourceInsert>(0.1);
            if (slot.plugin.uid == "test.source.loud")
                return std::make_unique<ConstantSourceInsert>(1.0);
            error = "unknown test insert";
            return nullptr;
        };
        return project::compileProjectGraph(project, tempo, options);
    }

    /// Renders `blocks` blocks and returns the last block's steady amplitude
    /// on channel 0 of the master.
    double renderLevel(project::CompiledProjectGraph& compiled, int blocks)
    {
        auto* effect = static_cast<CompressorEffect*>(
            compiled.insertStateFor(compressorSlot));
        REQUIRE(effect != nullptr);

        effect->setParameter(CompressorEffect::thresholdDb, -20.0);
        effect->setParameter(CompressorEffect::ratio, 20.0);
        effect->setParameter(CompressorEffect::attackMs, 0.1);
        effect->setParameter(CompressorEffect::releaseMs, 50.0);

        AudioBufferPool pool;
        pool.allocate(1, 2, blockSize);
        const AudioBufferView view = pool.buffer(0);

        double last = 0.0;
        for (int block = 0; block < blocks; ++block) {
            compiled.graph->process(view, blockSize,
                                    static_cast<FramePosition>(block) * blockSize);
            last = std::fabs(static_cast<double>(view.channel(0)[blockSize - 1]));
        }
        return last;
    }
};

} // namespace

TEST_CASE("a sidechain edge compiles into keyed ducking")
{
    SidechainFixture fixture;

    auto baseline = fixture.compile();
    REQUIRE(baseline);

    // The quiet source passes untouched. Each strip's constant-power centre
    // pan scales a channel by 1/√2, and the path crosses two strips: 0.1
    // arrives at the master as exactly 0.05.
    const double free = fixture.renderLevel(baseline, 20);
    CHECK(free == doctest::Approx(0.05).epsilon(0.01));

    REQUIRE(fixture.registry.execute(std::make_unique<app::ConnectMixerCommand>(
        fixture.kickStrip, fixture.bassStrip, false, 1.0, false, true)));

    auto keyed = fixture.compile();
    REQUIRE(keyed);
    CHECK(keyed.warnings.empty());

    const double ducked = fixture.renderLevel(keyed, 20);

    // The key reaches the detector at 1/√2 (its strip's centre pan):
    // -3.01 dB, which is 16.99 dB over threshold; ratio 20 takes
    // (1/20 - 1) × 16.99 = -16.14 dB. The quiet source under that reduction
    // is 0.05 × 10^(-16.14/20) ≈ 0.0078 — and nowhere near the key's own
    // level, which must not be audible at all.
    CHECK(ducked == doctest::Approx(0.0078).epsilon(0.05));

    auto* effect = static_cast<CompressorEffect*>(keyed.insertStateFor(fixture.compressorSlot));
    CHECK(effect->gainReductionDb() == doctest::Approx(16.14).epsilon(0.01));
}

TEST_CASE("the key never joins the audio path through the compiled graph")
{
    SidechainFixture fixture;

    REQUIRE(fixture.registry.execute(std::make_unique<app::ConnectMixerCommand>(
        fixture.kickStrip, fixture.bassStrip, false, 1.0, false, true)));

    auto compiled = fixture.compile();
    REQUIRE(compiled);

    // Ratio 1 disengages the gain computer entirely: the master must carry
    // the quiet source exactly (0.05 after two centre pans), not the loud key.
    auto* effect = static_cast<CompressorEffect*>(
        compiled.insertStateFor(fixture.compressorSlot));
    REQUIRE(effect != nullptr);

    AudioBufferPool pool;
    pool.allocate(1, 2, blockSize);
    const AudioBufferView view = pool.buffer(0);

    effect->setParameter(CompressorEffect::thresholdDb, -20.0);
    effect->setParameter(CompressorEffect::ratio, 1.0);

    double last = 0.0;
    for (int block = 0; block < 5; ++block) {
        compiled.graph->process(view, blockSize, static_cast<FramePosition>(block) * blockSize);
        last = std::fabs(static_cast<double>(view.channel(0)[blockSize - 1]));
    }

    CHECK(last == doctest::Approx(0.05).epsilon(1.0e-6));
}

TEST_CASE("a sidechain into a strip without a compressor warns and stays silent")
{
    SidechainFixture fixture;

    REQUIRE(fixture.registry.execute(std::make_unique<app::ConnectMixerCommand>(
        fixture.kickStrip, fixture.project.masterMixerNode(), false, 1.0, false, true)));

    auto compiled = fixture.compile();
    REQUIRE(compiled);

    bool warned = false;
    for (const std::string& warning : compiled.warnings)
        if (warning.find("sidechain") != std::string::npos)
            warned = true;
    CHECK(warned);
}
