#include "engine/instrument/Wavetable.h"

#include "engine/dsp/Fft.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace incdaw::engine {
namespace {

using dsp::Fft;

constexpr double pi = std::numbers::pi;

/// Inverse transform of a conjugate-symmetric spectrum, through the forward
/// FFT the engine already has.
///
///   ifft(X) = conj(fft(conj(X))) / N
///
/// and the result is real, so only the real half is kept. Adding an `inverse`
/// to Fft for one caller would be a second transform to keep correct.
void inverseRealTransform(const Fft& fft, std::vector<float>& real,
                          std::vector<float>& imaginary)
{
    for (float& value : imaginary)
        value = -value;

    fft.forward(real.data(), imaginary.data());

    const float scale = 1.0f / static_cast<float>(real.size());
    for (float& value : real)
        value *= scale;
}

// ── The recipes ──────────────────────────────────────────────────────────────
//
// Each is a function of (harmonic, frame, frameCount) returning an amplitude.
// A recipe is the whole of a table: no waveform data appears in this file.

double morph(std::size_t frame, std::size_t frameCount) noexcept
{
    return frameCount < 2 ? 0.0
                          : static_cast<double>(frame) / static_cast<double>(frameCount - 1);
}

/// Sine, triangle, sawtooth, square — the four shapes every synth starts
/// from, as one table so that position sweeps between them.
double basicRecipe(int harmonic, std::size_t frame, std::size_t frameCount) noexcept
{
    (void)frameCount;

    const double n = static_cast<double>(harmonic);
    const bool   odd = (harmonic % 2) == 1;

    switch (frame) {
        case 0:  return harmonic == 1 ? 1.0 : 0.0;                       // sine
        case 1:  return odd ? 1.0 / (n * n) : 0.0;                       // triangle
        case 2:  return 1.0 / n;                                         // sawtooth
        default: return odd ? 1.0 / n : 0.0;                             // square
    }
}

/// A pulse train narrowing from a square to a thin spike. The Fourier series
/// of a pulse of duty `d` is sin(pi n d) / (pi n), which is exactly what a
/// pulse-width knob sweeps.
double pulseRecipe(int harmonic, std::size_t frame, std::size_t frameCount) noexcept
{
    const double duty = 0.5 - 0.44 * morph(frame, frameCount);
    const double n    = static_cast<double>(harmonic);

    return std::sin(pi * n * duty) / (pi * n);
}

/// Sawtooth to square: the even harmonics fade out, which is the difference
/// between the two and a far more useful sweep than crossfading the shapes.
double evenFadeRecipe(int harmonic, std::size_t frame, std::size_t frameCount) noexcept
{
    const double t = morph(frame, frameCount);
    const double n = static_cast<double>(harmonic);

    const double amplitude = 1.0 / n;
    return (harmonic % 2) == 1 ? amplitude : amplitude * (1.0 - t);
}

/// A resonant peak walking up the harmonic series — a vowel-like sweep, and
/// the sound a wavetable is bought for.
double formantRecipe(int harmonic, std::size_t frame, std::size_t frameCount) noexcept
{
    const double t      = morph(frame, frameCount);
    const double centre = 1.0 + t * 23.0;
    const double width  = 2.5;
    const double n      = static_cast<double>(harmonic);

    const double offset = (n - centre) / width;
    return (1.0 / n) * (0.12 + std::exp(-0.5 * offset * offset));
}

/// A comb whose teeth close up across the table: hollow and metallic at one
/// end, full at the other.
double combRecipe(int harmonic, std::size_t frame, std::size_t frameCount) noexcept
{
    const double t = morph(frame, frameCount);
    const double n = static_cast<double>(harmonic);

    const double teeth = 1.0 + t * 7.0;
    return (1.0 / n) * (0.5 + 0.5 * std::cos(pi * n * teeth / 16.0));
}

} // namespace

std::size_t Wavetable::lengthOfLevel(std::size_t level) noexcept
{
    (void)level;
    return frameSize;
}

int Wavetable::harmonicsOfLevel(std::size_t level) noexcept
{
    const int shifted = maxHarmonics >> std::min(level, std::size_t{16});
    return std::max(shifted, 1);
}

std::size_t Wavetable::levelFor(double frequency, SampleRate sampleRate) noexcept
{
    const double nyquist = (sampleRate > 0.0 ? sampleRate : 48000.0) * 0.5;
    if (frequency <= 0.0)
        return 0;

    // How many harmonics fit under Nyquist at this pitch.
    const double allowed = nyquist / frequency;

    for (std::size_t level = 0; level < levelCount; ++level)
        if (static_cast<double>(harmonicsOfLevel(level)) <= allowed)
            return level;

    return levelCount - 1;
}

Wavetable::Wavetable(std::string name, std::size_t frames,
                     double (*amplitudeFor)(int, std::size_t, std::size_t))
    : name_(std::move(name)), frameCount_(std::max<std::size_t>(frames, 1))
{
    // Level offsets inside one frame's block, each with a guard sample so the
    // interpolator never wraps its index.
    std::size_t cursor = 0;
    for (std::size_t level = 0; level < levelCount; ++level) {
        offsetOfLevel_[level] = cursor;
        cursor += lengthOfLevel(level) + 1;
    }

    strideFrame_ = cursor;
    samples_.assign(strideFrame_ * frameCount_, 0.0f);

    Fft fft;
    fft.setSize(frameSize);

    std::vector<float> real(frameSize, 0.0f);
    std::vector<float> imaginary(frameSize, 0.0f);

    for (std::size_t frame = 0; frame < frameCount_; ++frame) {
        // Level 0 first, and its peak sets the normalisation for every level
        // of this frame — normalising each level separately would make the
        // sound change loudness as a note climbs into a narrower band.
        double normalise = 1.0;

        for (std::size_t level = 0; level < levelCount; ++level) {
            std::fill(real.begin(), real.end(), 0.0f);
            std::fill(imaginary.begin(), imaginary.end(), 0.0f);

            const int limit = std::min(harmonicsOfLevel(level), maxHarmonics - 1);

            for (int harmonic = 1; harmonic <= limit; ++harmonic) {
                const double amplitude = amplitudeFor(harmonic, frame, frameCount_);
                if (amplitude == 0.0)
                    continue;

                // Sine phase, and the conjugate bin that makes the result
                // real: X[n] = -i A/2, X[N-n] = +i A/2.
                const auto half = static_cast<float>(amplitude * 0.5);
                imaginary[static_cast<std::size_t>(harmonic)]              = -half;
                imaginary[frameSize - static_cast<std::size_t>(harmonic)] = half;
            }

            inverseRealTransform(fft, real, imaginary);

            if (level == 0) {
                float peak = 0.0f;
                for (const float value : real)
                    peak = std::max(peak, std::abs(value));

                normalise = peak > 1e-9f ? 1.0 / static_cast<double>(peak) : 1.0;
            }

            const std::size_t length = lengthOfLevel(level);

            float* destination = &samples_[frame * strideFrame_ + offsetOfLevel_[level]];
            for (std::size_t index = 0; index < length; ++index)
                destination[index] =
                    static_cast<float>(static_cast<double>(real[index]) * normalise);

            destination[length] = destination[0];   // guard
        }
    }
}

const float* Wavetable::frameLevel(std::size_t frame, std::size_t level) const noexcept
{
    return &samples_[frame * strideFrame_ + offsetOfLevel_[level]];
}

float Wavetable::sample(double position, std::size_t level, double phase) const noexcept
{
    const std::size_t safeLevel = std::min(level, levelCount - 1);
    const std::size_t length    = lengthOfLevel(safeLevel);

    double wrapped = phase - std::floor(phase);
    if (!(wrapped >= 0.0) || !(wrapped < 1.0))
        wrapped = 0.0;   // a NaN increment must not read outside the table

    const double  exact = wrapped * static_cast<double>(length);
    const auto    index = static_cast<std::size_t>(exact);
    const auto    frac  = static_cast<float>(exact - static_cast<double>(index));

    const double  spot      = std::clamp(position, 0.0, 1.0)
                            * static_cast<double>(frameCount_ - 1);
    const auto    frameLow  = static_cast<std::size_t>(spot);
    const std::size_t frameHigh = std::min(frameLow + 1, frameCount_ - 1);
    const auto    frameFrac = static_cast<float>(spot - static_cast<double>(frameLow));

    const float* low  = frameLevel(frameLow, safeLevel);
    const float* high = frameLevel(frameHigh, safeLevel);

    const float a = low[index] + (low[index + 1] - low[index]) * frac;
    const float b = high[index] + (high[index + 1] - high[index]) * frac;

    return a + (b - a) * frameFrac;
}

const std::vector<Wavetable>& wavetables()
{
    static const std::vector<Wavetable> tables = [] {
        std::vector<Wavetable> rows;
        rows.reserve(5);

        rows.emplace_back("Basic",   4, &basicRecipe);
        rows.emplace_back("Pulse",   8, &pulseRecipe);
        rows.emplace_back("Even Fade", 8, &evenFadeRecipe);
        rows.emplace_back("Formant", 8, &formantRecipe);
        rows.emplace_back("Comb",    8, &combRecipe);

        return rows;
    }();

    static_assert(wavetableCount == 5, "the declared count and the catalogue must agree");
    return tables;
}

} // namespace incdaw::engine
