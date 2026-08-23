#pragma once

// Waveshaper with a drawn transfer curve (A9).
//
// `incdaw.saturator` is a tanh and a mix, and it stays exactly that: a good
// default curve is worth having as its own effect. This is the other kind of
// waveshaper — the one where the curve IS the parameter. Nine control points
// spanning -1..+1, a Catmull-Rom spline through them, and whatever that
// spline says is what the audio thread applies.
//
// Two things make it honest rather than a toy:
//
//   · The points are ordinary automatable parameters, so a curve can be
//     automated, saved in a preset, and MIDI-mapped like anything else. A
//     shaper whose curve lives outside the parameter system would be the one
//     control in INCDAW that automation cannot reach.
//
//   · It oversamples. A drawn curve can have corners a tanh never has, and a
//     corner generates harmonics far above Nyquist that fold straight back
//     down. Shaping at two or four times the rate and filtering on the way
//     back is the difference between distortion and gravel.
//
// At the identity curve with no drive the effect passes the signal through
// bit-exact, by skipping the whole path rather than trusting an oversampler
// to be transparent.

#include "engine/dsp/effects/BuiltinEffect.h"

#include <array>
#include <cstddef>

namespace incdaw::engine::dsp {

/// Control points of the transfer curve, from x = -1 to x = +1.
inline constexpr std::size_t shaperPointCount = 9;

/// The x position of control point `index`.
[[nodiscard]] constexpr double shaperPointX(std::size_t index) noexcept
{
    return -1.0 + 2.0 * static_cast<double>(index)
                      / static_cast<double>(shaperPointCount - 1);
}

/// The curve's value at `x`, from the control points — a Catmull-Rom spline,
/// clamped to the end points outside -1..+1.
///
/// Free, and used by BOTH the audio thread's table build and any view that
/// draws the curve, for the same reason `eqMagnitudeDb` is free: a picture
/// that disagrees with the filter is worse than no picture.
[[nodiscard]] double shaperCurveAt(const double points[shaperPointCount], double x) noexcept;

/// Length of the halfband filter each oversampling stage runs. Odd, so it has
/// a centre tap; every second tap either side of it is zero, which is what
/// makes a halfband cheap.
inline constexpr int halfbandTaps = 31;

class WaveshaperEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t maxChannels = 8;

    /// Resolution of the table the audio thread reads. 1024 points over a
    /// curve that is already smooth is far finer than the ear resolves, and
    /// it keeps a spline evaluation out of the per-sample path.
    static constexpr std::size_t tableSize = 1024;

    /// Ids are frozen: they key the state blob and every saved preset. The
    /// points are contiguous from `pointBase` so a UI can walk them.
    enum Param : std::uint32_t {
        driveDb    = 0,
        mix        = 1,
        outputDb   = 2,
        oversample = 3,   ///< 0 → 1x · 1 → 2x · 2 → 4x
        pointBase  = 10,
    };

    WaveshaperEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Waveshaper"; }

private:
    /// A halfband FIR: cutoff at a quarter of the rate it runs at, which is
    /// what both halves of a 2x oversampler need. Every second tap either
    /// side of the centre is zero, so the convolution skips them.
    struct Halfband {
        static constexpr int taps   = halfbandTaps;
        static constexpr int centre = taps / 2;

        std::array<double, taps> history{};
        int cursor = 0;

        void reset() noexcept
        {
            history.fill(0.0);
            cursor = 0;
        }

        [[nodiscard]] double step(const std::array<double, taps>& coefficients,
                                  double input) noexcept;
    };

    /// One 2x stage: the filter that inserts samples and the one that removes
    /// them again.
    struct Stage {
        Halfband up, down;

        void reset() noexcept
        {
            up.reset();
            down.reset();
        }
    };

    void rebuildTableIfNeeded() noexcept;
    [[nodiscard]] double shapeThroughTable(double x) const noexcept;

    std::array<double, halfbandTaps> coefficients_{};

    std::array<double, tableSize + 1> table_{};
    std::array<double, shaperPointCount> cachedPoints_{};
    bool tableValid_ = false;

    /// Two stages give 4x; one gives 2x.
    std::array<std::array<Stage, 2>, maxChannels> stages_{};

    SampleRate sampleRate_ = 48000.0;
};

} // namespace incdaw::engine::dsp
