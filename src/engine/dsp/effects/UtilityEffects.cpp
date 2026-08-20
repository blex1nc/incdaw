#include "engine/dsp/effects/UtilityEffects.h"

#include <cmath>

namespace incdaw::engine::dsp {

namespace {

constexpr EffectParameter utilityParameters[] = {
    {UtilityEffect::gainDb,   "Gain",     -60.0, 24.0, 0.0, false},
    {UtilityEffect::pan,      "Pan",       -1.0,  1.0, 0.0, false},
    {UtilityEffect::width,    "Width",      0.0,  2.0, 1.0, false},
    {UtilityEffect::polarity, "Polarity",   0.0,  1.0, 0.0, true},
    {UtilityEffect::mono,     "Mono",       0.0,  1.0, 0.0, true},
};

} // namespace

UtilityEffect::UtilityEffect() : BuiltinEffect(utilityParameters, 5) {}

void UtilityEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const double gain     = dbToGain(valueAt(0)) * (valueAt(3) >= 0.5 ? -1.0 : 1.0);
    const double panValue = valueAt(1);
    const double widthVal = valueAt(2);
    const bool   toMono   = valueAt(4) >= 0.5;

    const std::size_t channels = context.output.channelCount();
    const FrameCount  frames   = context.frameCount;

    // Constant-power balance, normalised against its centre value so that
    // pan 0 is exactly unity — the null test holds the defaults to that.
    double panLeft  = 1.0;
    double panRight = 1.0;
    if (panValue != 0.0) {
        constexpr double quarterPi = 0.78539816339744830962;
        panLeft  = std::cos((panValue + 1.0) * quarterPi / 2.0) / std::cos(quarterPi / 2.0);
        panRight = std::sin((panValue + 1.0) * quarterPi / 2.0) / std::sin(quarterPi / 2.0);
    }

    if (channels >= 2) {
        Sample* left  = context.output.channel(0);
        Sample* right = context.output.channel(1);

        for (FrameCount frame = 0; frame < frames; ++frame) {
            double sampleLeft  = static_cast<double>(left[frame]);
            double sampleRight = static_cast<double>(right[frame]);

            if (toMono) {
                const double centre = (sampleLeft + sampleRight) * 0.5;
                sampleLeft = sampleRight = centre;
            } else if (widthVal != 1.0) {
                const double mid  = (sampleLeft + sampleRight) * 0.5;
                const double side = (sampleLeft - sampleRight) * 0.5 * widthVal;
                sampleLeft  = mid + side;
                sampleRight = mid - side;
            }

            left[frame]  = static_cast<Sample>(sampleLeft * gain * panLeft);
            right[frame] = static_cast<Sample>(sampleRight * gain * panRight);
        }

        // Any further channels get plain gain — width and pan are a stereo idea.
        for (std::size_t channel = 2; channel < channels; ++channel) {
            Sample* samples = context.output.channel(channel);
            for (FrameCount frame = 0; frame < frames; ++frame)
                samples[frame] = static_cast<Sample>(static_cast<double>(samples[frame]) * gain);
        }

        return;
    }

    for (std::size_t channel = 0; channel < channels; ++channel) {
        Sample* samples = context.output.channel(channel);
        for (FrameCount frame = 0; frame < frames; ++frame)
            samples[frame] = static_cast<Sample>(static_cast<double>(samples[frame]) * gain);
    }
}

AnalyzerEffect::AnalyzerEffect() : BuiltinEffect(nullptr, 0) {}

void AnalyzerEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;

    sampleRate_.store(sampleRate, std::memory_order_relaxed);

    fft_.setSize(fftSize);

    window_.assign(fftSize, 0.0f);
    for (std::size_t index = 0; index < fftSize; ++index)
        window_[index] = static_cast<float>(
            0.5 * (1.0 - std::cos(2.0 * M_PI * static_cast<double>(index)
                                  / static_cast<double>(fftSize - 1))));

    accumulate_.assign(fftSize, 0.0f);
    accumulated_ = 0;
    scratchReal_.assign(fftSize, 0.0f);
    scratchImaginary_.assign(fftSize, 0.0f);

    published_[0].assign(binCount, -160.0f);
    published_[1].assign(binCount, -160.0f);
    generation_.store(0, std::memory_order_release);
}

void AnalyzerEffect::publishSpectrum() noexcept
{
    for (std::size_t index = 0; index < fftSize; ++index) {
        scratchReal_[index]      = accumulate_[index] * window_[index];
        scratchImaginary_[index] = 0.0f;
    }

    fft_.forward(scratchReal_.data(), scratchImaginary_.data());

    // Hann coherent gain is 0.5; a full-scale sine lands at 0 dBFS.
    constexpr float scale = 4.0f / static_cast<float>(fftSize);
    constexpr float floor = 1.0e-8f;

    const std::uint64_t generation = generation_.load(std::memory_order_relaxed);
    std::vector<float>& target     = published_[(generation + 1) & 1];

    for (std::size_t bin = 0; bin < binCount; ++bin) {
        const float magnitude = std::sqrt(scratchReal_[bin] * scratchReal_[bin]
                                          + scratchImaginary_[bin] * scratchImaginary_[bin])
                              * scale;
        target[bin] = 20.0f * std::log10(magnitude > floor ? magnitude : floor);
    }

    generation_.store(generation + 1, std::memory_order_release);
}

void AnalyzerEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const std::size_t channels =
        context.output.channelCount() < maxChannels ? context.output.channelCount() : maxChannels;

    for (std::size_t channel = 0; channel < channels; ++channel) {
        const Sample* samples = context.output.channel(channel);

        float  peak = 0.0f;
        double sum  = 0.0;
        for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
            const float value = std::fabs(samples[frame]);
            peak = value > peak ? value : peak;
            sum += static_cast<double>(samples[frame]) * static_cast<double>(samples[frame]);
        }

        peak_[channel].store(peak, std::memory_order_relaxed);
        rms_[channel].store(
            context.frameCount > 0
                ? static_cast<float>(std::sqrt(sum / static_cast<double>(context.frameCount)))
                : 0.0f,
            std::memory_order_relaxed);
    }

    // ── The spectrum half: accumulate a mono downmix, transform when full.
    if (accumulate_.size() != fftSize)
        return;   // never prepared: levels only

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        float mono = 0.0f;
        for (std::size_t channel = 0; channel < channels; ++channel)
            mono += context.output.channel(channel)[frame];

        accumulate_[accumulated_] = channels > 0 ? mono / static_cast<float>(channels) : 0.0f;

        if (++accumulated_ == fftSize) {
            publishSpectrum();
            accumulated_ = 0;
        }
    }
}

bool AnalyzerEffect::readSpectrum(std::vector<float>& binsDb) const
{
    // Seqlock-style: copy the buffer the generation points at, then confirm
    // the generation did not move underneath the copy.
    for (int attempt = 0; attempt < 4; ++attempt) {
        const std::uint64_t before = generation_.load(std::memory_order_acquire);
        if (before == 0)
            return false;   // nothing published yet

        binsDb = published_[before & 1];

        if (generation_.load(std::memory_order_acquire) == before)
            return true;
    }

    return false;   // the audio thread kept lapping us; try next frame
}

// ── LoudnessMeterEffect ───────────────────────────────────────────────────────

namespace {

constexpr double pi = 3.14159265358979323846;

/// Energy → LUFS, with the BS.1770 offset that lands a 997 Hz full-scale
/// left-channel sine on -3.01 LKFS.
double lufsOf(double meanSquare) noexcept
{
    return meanSquare > 1.0e-12 ? -0.691 + 10.0 * std::log10(meanSquare) : -100.0;
}

} // namespace

LoudnessMeterEffect::LoudnessMeterEffect() : BuiltinEffect(nullptr, 0) {}

void LoudnessMeterEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;

    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    hopFrames_  = static_cast<FrameCount>(sampleRate_ / 10.0);
    if (hopFrames_ <= 0)
        hopFrames_ = 4800;

    // BS.1770-4's K-weighting from its analogue prototypes, so any sample
    // rate reproduces the spec's 48 kHz table (the constants are the spec's).
    {
        const double f0 = 1681.974450955533;
        const double g  = 3.999843853973347;
        const double q  = 0.7071752369554196;

        const double k  = std::tan(pi * f0 / sampleRate_);
        const double vh = std::pow(10.0, g / 20.0);
        const double vb = std::pow(vh, 0.4996667741545416);
        const double a0 = 1.0 + k / q + k * k;

        for (Biquad& stage : shelf_) {
            stage    = Biquad{};
            stage.b0 = (vh + vb * k / q + k * k) / a0;
            stage.b1 = 2.0 * (k * k - vh) / a0;
            stage.b2 = (vh - vb * k / q + k * k) / a0;
            stage.a1 = 2.0 * (k * k - 1.0) / a0;
            stage.a2 = (1.0 - k / q + k * k) / a0;
        }
    }
    {
        const double f0 = 38.13547087602444;
        const double q  = 0.5003270373238773;

        const double k  = std::tan(pi * f0 / sampleRate_);
        const double a0 = 1.0 + k / q + k * k;

        for (Biquad& stage : highpass_) {
            stage    = Biquad{};
            stage.b0 = 1.0;
            stage.b1 = -2.0;
            stage.b2 = 1.0;
            stage.a1 = 2.0 * (k * k - 1.0) / a0;
            stage.a2 = (1.0 - k / q + k * k) / a0;
        }
    }

    hopFilled_ = 0;
    hopEnergy_ = 0.0;
    hopCursor_ = 0;
    hopCount_  = 0;
    for (double& hop : hopHistory_)
        hop = 0.0;

    momentary_.store(lufsFloor, std::memory_order_relaxed);
    shortTerm_.store(lufsFloor, std::memory_order_relaxed);
    for (std::size_t bin = 0; bin < histogramBins; ++bin) {
        histogramCount_[bin].store(0, std::memory_order_relaxed);
        histogramPower_[bin].store(0.0, std::memory_order_relaxed);
    }
}

void LoudnessMeterEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    const std::size_t channels =
        context.output.channelCount() < maxChannels ? context.output.channelCount() : maxChannels;
    if (channels == 0 || hopFrames_ <= 0)
        return;

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        for (std::size_t channel = 0; channel < channels; ++channel) {
            const double sample =
                static_cast<double>(context.output.channel(channel)[frame]);
            const double weighted = highpass_[channel].step(shelf_[channel].step(sample));
            hopEnergy_ += weighted * weighted;
        }

        if (++hopFilled_ >= hopFrames_)
            completeHop();
    }
}

void LoudnessMeterEffect::completeHop() noexcept
{
    if (resetRequested_.exchange(false, std::memory_order_relaxed)) {
        hopCursor_ = 0;
        hopCount_  = 0;
        for (double& hop : hopHistory_)
            hop = 0.0;
        for (std::size_t bin = 0; bin < histogramBins; ++bin) {
            histogramCount_[bin].store(0, std::memory_order_relaxed);
            histogramPower_[bin].store(0.0, std::memory_order_relaxed);
        }
        momentary_.store(lufsFloor, std::memory_order_relaxed);
        shortTerm_.store(lufsFloor, std::memory_order_relaxed);
    }

    // Mean square of this hop, summed over channels (all weights are 1 for
    // left and right; surround weighting waits for surround).
    const double hopMean = hopEnergy_ / static_cast<double>(hopFrames_);
    hopEnergy_           = 0.0;
    hopFilled_           = 0;

    hopHistory_[hopCursor_] = hopMean;
    hopCursor_              = (hopCursor_ + 1) % shortTermHops;
    if (hopCount_ < shortTermHops)
        ++hopCount_;

    const auto windowMean = [this](std::size_t hops) noexcept {
        double sum = 0.0;
        for (std::size_t back = 0; back < hops; ++back) {
            const std::size_t index = (hopCursor_ + shortTermHops - 1 - back) % shortTermHops;
            sum += hopHistory_[index];
        }
        return sum / static_cast<double>(hops);
    };

    if (hopCount_ >= momentaryHops) {
        const double blockMean     = windowMean(momentaryHops);
        const double blockLoudness = lufsOf(blockMean);

        momentary_.store(blockLoudness, std::memory_order_relaxed);

        // Every hop completes one 400 ms gating block (75 % overlap). Blocks
        // over the -70 LUFS absolute gate enter the histogram.
        if (blockLoudness > -70.0) {
            const double position = (blockLoudness + 70.0) * 10.0;
            std::size_t  bin      = position <= 0.0 ? 0 : static_cast<std::size_t>(position);
            if (bin >= histogramBins)
                bin = histogramBins - 1;

            histogramCount_[bin].fetch_add(1, std::memory_order_relaxed);
            histogramPower_[bin].fetch_add(blockMean, std::memory_order_relaxed);
        }
    }

    if (hopCount_ >= shortTermHops)
        shortTerm_.store(lufsOf(windowMean(shortTermHops)), std::memory_order_relaxed);
}

double LoudnessMeterEffect::integratedLufs() const noexcept
{
    // First stage: everything over the absolute gate (the histogram's whole
    // population). Its mean sets the relative threshold 10 LU down.
    double        power = 0.0;
    std::uint64_t count = 0;

    for (std::size_t bin = 0; bin < histogramBins; ++bin) {
        count += histogramCount_[bin].load(std::memory_order_relaxed);
        power += histogramPower_[bin].load(std::memory_order_relaxed);
    }
    if (count == 0)
        return lufsFloor;

    const double threshold = lufsOf(power / static_cast<double>(count)) - 10.0;

    double        gatedPower = 0.0;
    std::uint64_t gatedCount = 0;

    for (std::size_t bin = 0; bin < histogramBins; ++bin) {
        const double binLoudness = -70.0 + (static_cast<double>(bin) + 0.5) / 10.0;
        if (binLoudness < threshold)
            continue;

        gatedCount += histogramCount_[bin].load(std::memory_order_relaxed);
        gatedPower += histogramPower_[bin].load(std::memory_order_relaxed);
    }

    return gatedCount > 0 ? lufsOf(gatedPower / static_cast<double>(gatedCount)) : lufsFloor;
}

} // namespace incdaw::engine::dsp
