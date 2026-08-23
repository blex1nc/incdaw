#pragma once

// Bar-synced time and volume gating (A13).
//
// The Gross Beat-shaped gap. Two curves drawn over one bar of the timeline:
//
//   · VOLUME is the easy half — a gain envelope that repeats every bar, which
//     is a gate, a tremolo, a sidechain-shaped duck, or a stutter's silence
//     depending on how it is drawn.
//
//   · TIME is the half that makes it interesting. The effect keeps the last
//     two bars of input, and the curve says how far BACK to read at each
//     point in the bar. A flat curve at zero is the present and the effect
//     does nothing. A curve that holds still replays one instant — that is a
//     stutter. A curve that walks backwards plays the bar in reverse. A curve
//     that steps back by an eighth and then catches up is the classic
//     Gross Beat repeat.
//
// The buffer is why time works at all: you cannot delay into the future, so
// "no offset" has to be the newest sample and every curve reads behind it.
//
// Bar position comes from the tempo map rather than from a tempo parameter,
// so the curves stay where they were written when the tempo changes.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/transport/TempoMap.h"

#include <array>
#include <cstddef>
#include <vector>

namespace incdaw::engine::dsp {

/// Control points per curve, over one bar. Sixteen is a sixteenth-note grid
/// at four-four, which is the resolution these gestures are written on.
inline constexpr std::size_t beatGatePoints = 16;

/// The value of a curve at bar position `phase` (0..1), from its points.
/// Linear between points and wrapping at the bar line — a curve that is
/// smooth across the loop is what keeps a stutter from clicking every bar.
[[nodiscard]] double beatGateCurveAt(const double points[beatGatePoints], double phase) noexcept;

class BeatGateEffect final : public BuiltinEffect {
public:
    static constexpr std::size_t maxChannels = 2;

    /// How far back the time curve may reach. Two bars at 60 bpm in four-four
    /// is eight seconds, which is the buffer this allocates.
    static constexpr double maxHistorySeconds = 8.0;

    /// Ids are frozen: they key the state blob and every saved preset. The
    /// two curves are contiguous runs so a view can walk them.
    enum Param : std::uint32_t {
        mix           = 0,
        timeAmount    = 1,   ///< how much of the time curve is applied, 0..1
        volumeAmount  = 2,
        smoothingMs   = 3,   ///< how quickly the read position may move
        bars          = 4,   ///< length of the pattern, in bars (stepped)
        timeBase      = 10,  ///< 16 points, each 0..1 bars of offset
        volumeBase    = 40,  ///< 16 points, each a gain 0..1
    };

    /// `tempoMap` must outlive the node — in a compiled project graph it is
    /// owned by the graph, exactly as the metronome's is. A null map means
    /// the effect has no bar to sync to, and it passes the signal through.
    explicit BeatGateEffect(const TempoMap* tempoMap = nullptr) noexcept;

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Beat Gate"; }

    /// The two curves as the audio thread reads them. For a view, and for the
    /// test that holds the rendered gain to the drawn one.
    [[nodiscard]] std::array<double, beatGatePoints> timeCurve() const noexcept;
    [[nodiscard]] std::array<double, beatGatePoints> volumeCurve() const noexcept;

private:
    /// Where in the pattern the frame at `position` falls, 0..1.
    [[nodiscard]] double phaseFor(FramePosition position, double barCount) const noexcept;

    const TempoMap* tempoMap_ = nullptr;

    std::vector<float> history_[maxChannels];
    std::size_t        historyWrite_ = 0;

    /// The read offset, smoothed: a curve that steps has to be allowed to
    /// step, but an unsmoothed jump inside a continuous passage clicks.
    double smoothedOffset_ = 0.0;

    SampleRate sampleRate_ = 48000.0;
};

} // namespace incdaw::engine::dsp
