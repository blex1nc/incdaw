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

/// EBU R128 / ITU-R BS.1770-4 loudness: momentary (400 ms), short-term (3 s)
/// and gated integrated LUFS, behind a bit-exact pass-through.
///
/// K-weighting is designed from the spec's analogue prototypes at whatever
/// sample rate `prepare` brings, so the meter reads the same at 44.1 and 48
/// kHz. Integration keeps a loudness histogram rather than every block, so a
/// day-long session costs the same memory as a minute.
class LoudnessMeterEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t maxChannels = 2;

    LoudnessMeterEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Loudness Meter"; }

    /// LUFS readouts; quieter than the measurable floor reads as -100.
    [[nodiscard]] double momentaryLufs() const noexcept
    {
        return momentary_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] double shortTermLufs() const noexcept
    {
        return shortTerm_.load(std::memory_order_relaxed);
    }

    /// Gated integrated loudness per BS.1770-4: blocks over -70 LUFS set a
    /// relative threshold 10 LU below their mean; the answer is the mean of
    /// what passes. Computed from the histogram on demand.
    [[nodiscard]] double integratedLufs() const noexcept;

    /// Restarts the integration, e.g. when the transport jumps to the top.
    void resetIntegration() noexcept { resetRequested_.store(true, std::memory_order_relaxed); }

private:
    struct Biquad {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        [[nodiscard]] double step(double input) noexcept
        {
            const double output = b0 * input + z1;
            z1                  = b1 * input - a1 * output + z2;
            z2                  = b2 * input - a2 * output;
            return output;
        }
    };

    static constexpr double lufsFloor = -100.0;

    /// 100 ms hops; 4 make a momentary window, 30 a short-term one.
    static constexpr std::size_t momentaryHops = 4;
    static constexpr std::size_t shortTermHops = 30;

    /// Histogram of gating-block loudness, 0.1 LU bins from -70 to 0 LUFS.
    static constexpr std::size_t histogramBins = 700;

    void completeHop() noexcept;

    Biquad shelf_[maxChannels];
    Biquad highpass_[maxChannels];

    SampleRate  sampleRate_    = 48000.0;
    FrameCount  hopFrames_     = 4800;
    FrameCount  hopFilled_     = 0;
    double      hopEnergy_     = 0.0;   ///< running sum of K-weighted squares, all channels

    double      hopHistory_[shortTermHops] = {};
    std::size_t hopCursor_ = 0;
    std::size_t hopCount_  = 0;

    std::atomic<double> momentary_{lufsFloor};
    std::atomic<double> shortTerm_{lufsFloor};
    std::atomic<bool>   resetRequested_{false};

    std::atomic<std::uint64_t> histogramCount_[histogramBins]{};
    std::atomic<double>        histogramPower_[histogramBins]{};
};

// ── Catalogue registrar ──────────────────────────────────────────────────────

struct EffectCatalogueEntry;

/// Utility, analyzer, loudness meter.
void registerUtilityEffects(std::vector<EffectCatalogueEntry>& rows);

} // namespace incdaw::engine::dsp
