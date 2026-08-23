#pragma once

// Linkwitz-Riley crossover pieces, shared by everything that has to split a
// signal and put it back together again.
//
// The identity every user of this header depends on: a fourth-order
// Linkwitz-Riley lowpass and highpass at the same frequency SUM TO AN
// ALLPASS. Magnitude flat, phase shifted — which is why a split can be
// rebuilt without a hole at the crossover, and also why the rebuilt signal is
// not bit-identical to the original. An effect that must null at its
// transparent settings therefore skips its split entirely rather than
// pretending the sum is an identity.
//
// A fourth-order half is two identical Butterworth sections in series; that
// is the whole construction.

#include "engine/core/Time.h"
#include "engine/dsp/effects/ToneEffects.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace incdaw::engine::dsp {

/// Butterworth Q. Two sections at this Q make one Linkwitz-Riley half.
inline constexpr double butterworthQ = 0.70710678118654752;

[[nodiscard]] inline BiquadCoefficients butterworthLowpass(double frequency,
                                                           SampleRate rate) noexcept
{
    const double w0    = 2.0 * std::numbers::pi
                       * std::clamp(frequency, 10.0, rate * 0.45) / rate;
    const double cosw  = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * butterworthQ);
    const double a0    = 1.0 + alpha;

    BiquadCoefficients c;
    c.b0 = ((1.0 - cosw) * 0.5) / a0;
    c.b1 = (1.0 - cosw) / a0;
    c.b2 = c.b0;
    c.a1 = (-2.0 * cosw) / a0;
    c.a2 = (1.0 - alpha) / a0;
    return c;
}

[[nodiscard]] inline BiquadCoefficients butterworthHighpass(double frequency,
                                                            SampleRate rate) noexcept
{
    const double w0    = 2.0 * std::numbers::pi
                       * std::clamp(frequency, 10.0, rate * 0.45) / rate;
    const double cosw  = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * butterworthQ);
    const double a0    = 1.0 + alpha;

    BiquadCoefficients c;
    c.b0 = ((1.0 + cosw) * 0.5) / a0;
    c.b1 = -(1.0 + cosw) / a0;
    c.b2 = c.b0;
    c.a1 = (-2.0 * cosw) / a0;
    c.a2 = (1.0 - alpha) / a0;
    return c;
}

/// One biquad's state, transposed direct form II — the form the EQ uses, so
/// nothing in the tree has a second idea of what a biquad is.
struct BiquadSection {
    double z1 = 0.0, z2 = 0.0;

    [[nodiscard]] double step(const BiquadCoefficients& c, double input) noexcept
    {
        const double output = c.b0 * input + z1;
        z1 = c.b1 * input - c.a1 * output + z2;
        z2 = c.b2 * input - c.a2 * output;
        return output;
    }

    void reset() noexcept { z1 = 0.0; z2 = 0.0; }
};

/// A fourth-order Linkwitz-Riley half: two identical sections in series.
struct LinkwitzRileyHalf {
    BiquadSection first, second;

    [[nodiscard]] double step(const BiquadCoefficients& c, double input) noexcept
    {
        return second.step(c, first.step(c, input));
    }

    void reset() noexcept { first.reset(); second.reset(); }
};

} // namespace incdaw::engine::dsp
