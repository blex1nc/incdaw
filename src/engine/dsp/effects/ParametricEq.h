#pragma once

// Parametric EQ (A10) — eight bands, selectable types, one shared design.
//
// This does NOT replace `incdaw.eq` or `incdaw.tone`. Those two are one
// three-band tone stack wearing two faces, and a tone stack is a different
// instrument from a parametric: it is the thing you reach for when you want
// "a bit more bottom", not the thing you reach for when a resonance at 340 Hz
// has to go. Widening the three-band EQ to eight would also have taken the
// Tone panel's face away from it and rewritten every EQ preset already on
// somebody's disk, for no gain to either tool.
//
// The property that carries over from the three-band EQ is the one that
// matters: a band's coefficients and the curve a view draws come from THE
// SAME function. A picture that disagrees with the filter is worse than no
// picture, and there is a test for it.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/dsp/effects/ToneEffects.h"

#include <array>
#include <cstddef>

namespace incdaw::engine::dsp {

/// Bands in a parametric EQ. Eight is what a mastering chain asks for and
/// twice what a channel strip does; more would be a scrolling list rather
/// than a curve anyone can read.
inline constexpr std::size_t parametricBandCount = 8;

/// What a band does. `off` is coefficient-identity, which is what lets a
/// defaulted EQ null against its input.
enum class ParametricBandType : int {
    off       = 0,
    lowShelf  = 1,
    peak      = 2,
    highShelf = 3,
    lowPass   = 4,
    highPass  = 5,
    notch     = 6,
};

inline constexpr int parametricBandTypeCount = 7;

/// The name of a band type, for menus. Never nullptr.
[[nodiscard]] const char* parametricBandTypeName(ParametricBandType type) noexcept;

/// One band's settings, in plain units.
struct ParametricBand {
    ParametricBandType type      = ParametricBandType::off;
    double             frequency = 1000.0;
    double             gainDb    = 0.0;
    double             q         = 0.707;
};

/// Designs one band. An `off` band, and a shelf or peak at exactly 0 dB,
/// return the identity — the same short circuit the three-band EQ makes, and
/// for the same reason.
[[nodiscard]] BiquadCoefficients designParametricBand(const ParametricBand& band,
                                                      SampleRate sampleRate) noexcept;

/// Combined magnitude response of the bands at `frequency`, in dB — what a
/// curve view draws, from the same design the audio thread runs.
[[nodiscard]] double
parametricMagnitudeDb(const std::array<ParametricBand, parametricBandCount>& bands,
                      SampleRate sampleRate, double frequency) noexcept;

class ParametricEqEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t maxChannels = 8;
    static constexpr std::size_t bandCount   = parametricBandCount;

    /// Ids are frozen: they key the state blob and every saved preset.
    /// A band's four controls live at `bandBase + band * bandStride + offset`,
    /// contiguous so a curve view can walk them.
    enum Param : std::uint32_t {
        outputDb = 0,
    };

    enum BandOffset : std::uint32_t {
        bandType      = 0,
        bandFrequency = 1,
        bandGainDb    = 2,
        bandQ         = 3,
    };

    static constexpr std::uint32_t bandBase   = 10;
    static constexpr std::uint32_t bandStride = 10;

    [[nodiscard]] static constexpr std::uint32_t bandParameter(std::size_t band,
                                                               BandOffset  offset) noexcept
    {
        return bandBase + static_cast<std::uint32_t>(band) * bandStride
             + static_cast<std::uint32_t>(offset);
    }

    ParametricEqEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Parametric EQ"; }

    /// The bands as the audio thread currently sees them. For a view, and for
    /// the test that holds the drawn curve to the rendered one.
    [[nodiscard]] std::array<ParametricBand, bandCount> bands() const noexcept;

private:
    struct State {
        double z1 = 0.0, z2 = 0.0;
    };

    std::array<BiquadCoefficients, bandCount> coefficients_{};
    std::array<ParametricBand, bandCount>     cached_{};
    bool                                      designed_ = false;

    std::array<std::array<State, maxChannels>, bandCount> states_{};

    SampleRate sampleRate_ = 48000.0;
};

} // namespace incdaw::engine::dsp
