#pragma once

#include "engine/dsp/effects/BuiltinEffect.h"

#include <atomic>

namespace incdaw::engine::dsp {

/// Gain, balance, width, polarity, mono — the channel-strip chores as one
/// insert. At its defaults it is exactly transparent, which the null test
/// holds it to.
class UtilityEffect final : public BuiltinEffect {
public:
    enum Param : std::uint32_t { gainDb = 0, pan = 1, width = 2, polarity = 3, mono = 4 };

    UtilityEffect();

    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Utility"; }
};

/// A bit-exact pass-through that measures: per-channel peak and RMS of the
/// last block, published through atomics the UI may read from any thread.
/// Deliberately parameterless — an analyzer that changed the signal would be
/// lying about it.
class AnalyzerEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t maxChannels = 2;

    AnalyzerEffect();

    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Analyzer"; }

    [[nodiscard]] float peak(std::size_t channel) const noexcept
    {
        return channel < maxChannels ? peak_[channel].load(std::memory_order_relaxed) : 0.0f;
    }

    [[nodiscard]] float rms(std::size_t channel) const noexcept
    {
        return channel < maxChannels ? rms_[channel].load(std::memory_order_relaxed) : 0.0f;
    }

private:
    std::atomic<float> peak_[maxChannels]{};
    std::atomic<float> rms_[maxChannels]{};
};

} // namespace incdaw::engine::dsp
