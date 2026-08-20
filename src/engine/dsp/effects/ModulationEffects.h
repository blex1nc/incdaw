#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <vector>

namespace incdaw::engine::dsp {

/// Chorus: two LFO-modulated delay taps in quadrature around a 15 ms centre,
/// blended with the dry signal. Mix 0 — the default — is bit-exact dry,
/// which is this suite's null-test convention.
class ChorusEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { rateHz = 0, depthMs = 1, mix = 2 };

    ChorusEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Chorus"; }

private:
    static constexpr std::size_t maxChannels  = 2;
    static constexpr double      centreMs     = 15.0;
    static constexpr double      maxDepthMs   = 10.0;

    std::vector<Sample> line_[maxChannels];
    std::size_t         mask_  = 0;
    std::size_t         write_ = 0;
    double              phase_ = 0.0;
    SampleRate          sampleRate_ = 48000.0;
};

/// Flanger: one short swept delay with feedback, summed with the dry path.
/// Mix 0 nulls.
class FlangerEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { rateHz = 0, depthMs = 1, feedback = 2, mix = 3 };

    FlangerEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Flanger"; }

private:
    static constexpr std::size_t maxChannels = 2;
    static constexpr double      baseMs      = 1.0;
    static constexpr double      maxDepthMs  = 5.0;

    std::vector<Sample> line_[maxChannels];
    Sample              held_[maxChannels] = {};   ///< feedback memory
    std::size_t         mask_  = 0;
    std::size_t         write_ = 0;
    double              phase_ = 0.0;
    SampleRate          sampleRate_ = 48000.0;
};

/// Phaser: four cascaded first-order allpasses swept by an LFO, with
/// feedback around the chain. Mix 0 nulls.
class PhaserEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { rateHz = 0, feedback = 1, mix = 2 };

    PhaserEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Phaser"; }

private:
    static constexpr std::size_t maxChannels = 2;
    static constexpr std::size_t stages      = 4;

    double     state_[maxChannels][stages] = {};
    Sample     held_[maxChannels]          = {};
    double     phase_                      = 0.0;
    SampleRate sampleRate_                 = 48000.0;
};

} // namespace incdaw::engine::dsp
