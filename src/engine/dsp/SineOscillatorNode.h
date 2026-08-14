#pragma once

#include "engine/graph/Node.h"

#include <atomic>

namespace incdaw::engine::dsp {

/// A sine generator.
///
/// Exists for two reasons beyond making noise: it is the signal the Phase 2
/// exit criterion is measured with, and it is the reference source for the
/// golden-file audio tests (docs/TESTING.md §4) — a pure tone makes any
/// unintended gain, phase, or sample-rate error obvious.
///
/// Phase is advanced as a fraction of a cycle rather than in radians so that it
/// can be wrapped exactly, without the slow drift that accumulates when a
/// radian phase is reduced modulo 2*pi.
class SineOscillatorNode final : public Node {
public:
    explicit SineOscillatorNode(double frequencyHz = 440.0, Sample amplitude = Sample{0.25}) noexcept
        : frequency_(frequencyHz), amplitude_(amplitude) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override
    {
        (void)maxBlockSize;
        sampleRate_ = sampleRate;
        phase_      = 0.0;
    }

    void process(const ProcessContext& context) noexcept override;

    /// Realtime-safe: read by the audio thread, written by the UI thread.
    void setFrequency(double hertz) noexcept { frequency_.store(hertz, std::memory_order_relaxed); }
    void setAmplitude(Sample gain) noexcept  { amplitude_.store(gain, std::memory_order_relaxed); }

    [[nodiscard]] double phase() const noexcept { return phase_; }
    [[nodiscard]] const char* name() const noexcept override { return "SineOscillator"; }

private:
    std::atomic<double> frequency_{440.0};
    std::atomic<Sample> amplitude_{Sample{0.25}};
    SampleRate          sampleRate_ = 0.0;
    double              phase_      = 0.0;   ///< cycles, in [0, 1)
};

} // namespace incdaw::engine::dsp
