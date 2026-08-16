#pragma once

#include "engine/dsp/Fft.h"
#include "engine/dsp/effects/BuiltinEffect.h"

#include <atomic>
#include <cstdint>
#include <vector>

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

    /// Analysis length: 2048 mono samples, Hann-windowed — ~23 Hz bins at
    /// 48 kHz, which is UI resolution, not measurement gear.
    static constexpr std::size_t fftSize  = 2048;
    static constexpr std::size_t binCount = fftSize / 2 + 1;

    AnalyzerEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
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

    /// Copies the latest published magnitude spectrum (dBFS per bin, DC to
    /// Nyquist) into `binsDb`. UI thread; false until the first full
    /// analysis window has been processed. `out` may allocate — the audio
    /// thread's half never does.
    [[nodiscard]] bool readSpectrum(std::vector<float>& binsDb) const;

    [[nodiscard]] SampleRate analysisSampleRate() const noexcept
    {
        return sampleRate_.load(std::memory_order_relaxed);
    }

private:
    void publishSpectrum() noexcept;

    std::atomic<float> peak_[maxChannels]{};
    std::atomic<float> rms_[maxChannels]{};

    // ── The spectrum half. All storage sized in prepare. ────────────────
    Fft                fft_;
    std::vector<float> window_;
    std::vector<float> accumulate_;   ///< mono downmix, filling toward fftSize
    std::size_t        accumulated_ = 0;
    std::vector<float> scratchReal_;
    std::vector<float> scratchImaginary_;

    /// Double-buffered publication: the audio thread writes the buffer the
    /// generation does NOT point at, then bumps the generation. An odd/even
    /// generation picks the buffer; the UI copies and re-checks, seqlock
    /// style.
    std::vector<float>         published_[2];
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<double>        sampleRate_{48000.0};
};

} // namespace incdaw::engine::dsp
