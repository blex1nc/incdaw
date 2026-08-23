// Band-limited wavetables (A1).
//
// One property carries this file: a wavetable read back at ANY musical pitch
// must not alias. That is the difference between a wavetable synth and a
// wavetable-shaped noise generator, and it is invisible in a waveform plot —
// only a spectrum shows it. So the tests here are spectral, and the reference
// waveform is summed harmonic by harmonic, independently of the FFT path the
// tables are actually built with.

#include "doctest.h"

#include "engine/dsp/Fft.h"
#include "engine/instrument/Wavetable.h"

#include <cmath>
#include <numbers>
#include <vector>

using namespace incdaw;
using engine::Wavetable;

namespace {

constexpr double pi = std::numbers::pi;

/// A sawtooth summed one harmonic at a time — the textbook definition, with
/// no transform anywhere near it. This is the independent reference the
/// constitution asks a DSP block to be checked against.
std::vector<double> additiveSawtooth(std::size_t length, int harmonics)
{
    std::vector<double> samples(length, 0.0);

    for (std::size_t index = 0; index < length; ++index) {
        const double phase = static_cast<double>(index) / static_cast<double>(length);

        double sum = 0.0;
        for (int harmonic = 1; harmonic <= harmonics; ++harmonic)
            sum += std::sin(2.0 * pi * static_cast<double>(harmonic) * phase)
                 / static_cast<double>(harmonic);

        samples[index] = sum;
    }

    double peak = 0.0;
    for (const double value : samples)
        peak = std::max(peak, std::abs(value));

    if (peak > 0.0)
        for (double& value : samples)
            value /= peak;

    return samples;
}

/// Magnitude spectrum of `samples`, whose length must be a power of two.
std::vector<double> magnitudes(const std::vector<float>& samples)
{
    engine::dsp::Fft fft;
    fft.setSize(samples.size());

    std::vector<float> real      = samples;
    std::vector<float> imaginary(samples.size(), 0.0f);

    fft.forward(real.data(), imaginary.data());

    std::vector<double> result(samples.size() / 2 + 1, 0.0);
    for (std::size_t bin = 0; bin < result.size(); ++bin)
        result[bin] = std::hypot(static_cast<double>(real[bin]),
                                 static_cast<double>(imaginary[bin]));

    return result;
}

const Wavetable& tableNamed(const char* name)
{
    for (const Wavetable& table : engine::wavetables())
        if (table.name() == name)
            return table;

    REQUIRE_MESSAGE(false, "no such table");
    return engine::wavetables().front();
}

} // namespace

TEST_CASE("the catalogue is built and every table has frames")
{
    REQUIRE_FALSE(engine::wavetables().empty());

    for (const Wavetable& table : engine::wavetables()) {
        CAPTURE(table.name());
        CHECK(table.frameCount() >= 2);
        CHECK_FALSE(table.name().empty());
    }
}

TEST_CASE("the sawtooth frame matches a harmonic sum computed independently")
{
    const Wavetable& basic = tableNamed("Basic");

    // Frame 2 of the basic set is the sawtooth; position picks it exactly.
    const double position = 2.0 / static_cast<double>(basic.frameCount() - 1);

    const std::vector<double> reference =
        additiveSawtooth(Wavetable::frameSize, Wavetable::maxHarmonics - 1);

    double worst = 0.0;
    for (std::size_t index = 0; index < Wavetable::frameSize; ++index) {
        const double phase = static_cast<double>(index)
                           / static_cast<double>(Wavetable::frameSize);
        const double actual = static_cast<double>(basic.sample(position, 0, phase));

        worst = std::max(worst, std::abs(actual - reference[index]));
    }

    CHECK(worst < 1e-4);
}

TEST_CASE("every frame is normalised to unity peak")
{
    for (const Wavetable& table : engine::wavetables()) {
        for (std::size_t frame = 0; frame < table.frameCount(); ++frame) {
            const double position = table.frameCount() < 2
                                        ? 0.0
                                        : static_cast<double>(frame)
                                              / static_cast<double>(table.frameCount() - 1);

            double peak = 0.0;
            for (std::size_t index = 0; index < Wavetable::frameSize; ++index) {
                const double phase = static_cast<double>(index)
                                   / static_cast<double>(Wavetable::frameSize);
                peak = std::max(peak, std::abs(static_cast<double>(
                                          table.sample(position, 0, phase))));
            }

            CAPTURE(table.name());
            CAPTURE(frame);
            CHECK(peak == doctest::Approx(1.0).epsilon(0.01));
        }
    }
}

TEST_CASE("a mip level contains nothing above its own harmonic limit")
{
    const Wavetable& formant = tableNamed("Formant");

    for (std::size_t level = 0; level < Wavetable::levelCount; ++level) {
        const std::size_t length   = Wavetable::lengthOfLevel(level);
        const int         harmonics = Wavetable::harmonicsOfLevel(level);

        std::vector<float> cycle(length, 0.0f);
        for (std::size_t index = 0; index < length; ++index)
            cycle[index] = formant.sample(1.0, level,
                                          static_cast<double>(index)
                                              / static_cast<double>(length));

        const std::vector<double> spectrum = magnitudes(cycle);

        double fundamental = 0.0;
        for (std::size_t bin = 1; bin <= static_cast<std::size_t>(harmonics)
                                  && bin < spectrum.size(); ++bin)
            fundamental = std::max(fundamental, spectrum[bin]);

        double above = 0.0;
        for (std::size_t bin = static_cast<std::size_t>(harmonics) + 1;
             bin < spectrum.size(); ++bin)
            above = std::max(above, spectrum[bin]);

        CAPTURE(level);
        CAPTURE(harmonics);

        // 80 dB down is silence by any musical standard.
        CHECK(above < fundamental * 1e-4);
    }
}

TEST_CASE("the level chosen for a pitch keeps every harmonic under Nyquist")
{
    constexpr double rate = 48000.0;

    for (int key = 0; key <= 127; ++key) {
        const double frequency = 440.0 * std::pow(2.0, (key - 69) / 12.0);
        const std::size_t level = Wavetable::levelFor(frequency, rate);

        CAPTURE(key);
        CAPTURE(frequency);

        const double highest =
            frequency * static_cast<double>(Wavetable::harmonicsOfLevel(level));

        // The top level holds a single harmonic; above ~24 kHz even that is
        // over Nyquist, and there is nothing left to drop.
        if (level + 1 < Wavetable::levelCount)
            CHECK(highest <= rate * 0.5 + 1e-6);

        // And it is the WIDEST level that fits: one step down would not.
        if (level > 0) {
            const double wider =
                frequency * static_cast<double>(Wavetable::harmonicsOfLevel(level - 1));
            CHECK(wider > rate * 0.5);
        }
    }
}

TEST_CASE("playing a bright table high up does not alias")
{
    // 2 kHz is a fundamental a wavetable synth really is asked for, and a
    // naive table read at it folds a dozen harmonics back down.
    constexpr double rate        = 48000.0;
    constexpr std::size_t frames = 8192;

    // Exactly on a bin, and its harmonics with it: a rectangular window over
    // an off-bin tone smears every harmonic across the spectrum, and the
    // measurement would then be of the window rather than of the table.
    constexpr double frequency = 341.0 * rate / static_cast<double>(frames);

    const Wavetable& basic = tableNamed("Basic");
    const std::size_t level = Wavetable::levelFor(frequency, rate);

    const double position = 2.0 / static_cast<double>(basic.frameCount() - 1);   // sawtooth

    std::vector<float> rendered(frames, 0.0f);
    double             phase = 0.0;
    const double       increment = frequency / rate;

    for (std::size_t index = 0; index < frames; ++index) {
        rendered[index] = basic.sample(position, level, phase);
        phase += increment;
        if (phase >= 1.0)
            phase -= 1.0;
    }

    const std::vector<double> spectrum = magnitudes(rendered);
    const double binHz = rate / static_cast<double>(frames);

    double fundamental = 0.0;
    double alias       = 0.0;

    for (std::size_t bin = 1; bin < spectrum.size(); ++bin) {
        const double hz = static_cast<double>(bin) * binHz;

        // Distance to the nearest harmonic of the fundamental.
        const double ratio    = hz / frequency;
        const double nearest  = std::round(ratio);
        const bool   harmonic = nearest >= 1.0 && std::abs(ratio - nearest) < 0.02;

        if (harmonic) {
            fundamental = std::max(fundamental, spectrum[bin]);
            continue;
        }

        alias = std::max(alias, spectrum[bin]);
    }

    CAPTURE(fundamental);
    CAPTURE(alias);

    // Everything that is not a harmonic of the note is at least 60 dB down.
    CHECK(alias < fundamental * 1e-3);
}

TEST_CASE("position morphs between frames rather than stepping")
{
    const Wavetable& pulse = tableNamed("Pulse");
    REQUIRE(pulse.frameCount() >= 2);

    const double step = 1.0 / static_cast<double>(pulse.frameCount() - 1);

    // Halfway between the first two frames is their average, by construction.
    for (std::size_t index = 0; index < 64; ++index) {
        const double phase = static_cast<double>(index) / 64.0;

        const auto low  = static_cast<double>(pulse.sample(0.0, 0, phase));
        const auto high = static_cast<double>(pulse.sample(step, 0, phase));
        const auto mid  = static_cast<double>(pulse.sample(step * 0.5, 0, phase));

        CHECK(mid == doctest::Approx((low + high) * 0.5).epsilon(1e-5));
    }
}

TEST_CASE("the table wraps cleanly and survives nonsense input")
{
    const Wavetable& basic = tableNamed("Basic");

    // Just before the wrap and just after it are neighbours, not a cliff.
    const auto before = static_cast<double>(basic.sample(0.0, 0, 1.0 - 1e-9));
    const auto after  = static_cast<double>(basic.sample(0.0, 0, 0.0));
    CHECK(std::abs(before - after) < 1e-3);

    // Out-of-range position and phase are clamped or wrapped, never read out
    // of the table — an instrument fed a NaN increment must go quiet, not
    // crash the audio thread.
    CHECK(std::isfinite(basic.sample(-5.0, 0, 3.7)));
    CHECK(std::isfinite(basic.sample(9.0, 99, -2.5)));
    CHECK(std::isfinite(basic.sample(0.5, 0, std::nan(""))));
}
