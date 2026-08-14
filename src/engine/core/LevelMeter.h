#pragma once

#include "engine/core/AudioBuffer.h"
#include "engine/core/Time.h"

#include <atomic>
#include <cmath>

namespace incdaw::engine {

/// Peak and RMS measurement for one signal, written by the audio thread and
/// read by the UI.
///
/// The transfer is a relaxed atomic store per block rather than a queue: a
/// meter is a *latest value*, not a stream. A dropped update is invisible at
/// 30 Hz, and a lock or a queue on the audio thread to protect a number the
/// reader is about to overwrite would be the wrong trade twice over.
///
/// RMS is accumulated over a rolling window rather than per block, so the
/// reading does not depend on the device's buffer size. That window is also
/// what a LUFS meter needs (CLAUDE.md §11 asks for LUFS-ready, not LUFS now):
/// K-weighting and gating go in front of this, they do not replace it.
class LevelMeter {
public:
    /// Peak decay, in decibels per second. Slow enough to read, fast enough to
    /// follow the music.
    static constexpr double peakDecayDbPerSecond = 20.0;

    /// RMS integration time. 300 ms is the usual "short term" window.
    static constexpr double rmsWindowSeconds = 0.3;

    void prepare(SampleRate sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        windowFrames_ = static_cast<FrameCount>(rmsWindowSeconds * sampleRate_);
        if (windowFrames_ < 1)
            windowFrames_ = 1;

        squareSum_ = 0.0;
        windowFilled_ = 0;
        heldPeak_ = 0.0f;

        peak_.store(0.0f, std::memory_order_relaxed);
        rms_.store(0.0f, std::memory_order_relaxed);
    }

    /// Measures one block. Realtime-safe: no allocation, no branching on data.
    void measure(const AudioBufferView& buffer, FrameCount frameCount) noexcept
    {
        if (buffer.isEmpty() || frameCount <= 0)
            return;

        Sample blockPeak = 0.0f;
        double sum       = 0.0;

        for (std::size_t channel = 0; channel < buffer.channelCount(); ++channel) {
            const Sample* samples = buffer.channel(channel);

            for (FrameCount frame = 0; frame < frameCount; ++frame) {
                const Sample value = samples[frame];
                const Sample magnitude = value < 0.0f ? -value : value;

                if (magnitude > blockPeak)
                    blockPeak = magnitude;

                sum += static_cast<double>(value) * static_cast<double>(value);
            }
        }

        const auto samplesSeen = static_cast<double>(frameCount)
                               * static_cast<double>(buffer.channelCount());

        // A one-pole approximation of a sliding window: exact enough for a
        // meter, and it costs no history buffer on the audio thread.
        const double window = static_cast<double>(windowFrames_);
        const double weight = static_cast<double>(frameCount) / window;
        const double blockMeanSquare = samplesSeen > 0.0 ? sum / samplesSeen : 0.0;

        squareSum_ += (blockMeanSquare - squareSum_) * (weight < 1.0 ? weight : 1.0);

        if (windowFilled_ < windowFrames_)
            windowFilled_ += frameCount;

        // Peak falls at a fixed rate rather than being replaced, so a transient
        // stays visible long enough to see.
        const double seconds = static_cast<double>(frameCount) / sampleRate_;
        const auto   decay   = static_cast<Sample>(std::pow(10.0, -peakDecayDbPerSecond * seconds / 20.0));

        heldPeak_ = blockPeak > heldPeak_ * decay ? blockPeak : heldPeak_ * decay;

        peak_.store(heldPeak_, std::memory_order_relaxed);
        rms_.store(static_cast<Sample>(std::sqrt(squareSum_)), std::memory_order_relaxed);
    }

    /// Latest peak, 0..1 linear. Safe to call from any thread.
    [[nodiscard]] Sample peak() const noexcept { return peak_.load(std::memory_order_relaxed); }

    /// Latest RMS, 0..1 linear.
    [[nodiscard]] Sample rms() const noexcept { return rms_.load(std::memory_order_relaxed); }

    void reset() noexcept { prepare(sampleRate_); }

private:
    SampleRate          sampleRate_   = 48000.0;
    FrameCount          windowFrames_ = 1;
    FrameCount          windowFilled_ = 0;
    double              squareSum_    = 0.0;
    Sample              heldPeak_     = 0.0f;

    std::atomic<Sample> peak_{0.0f};
    std::atomic<Sample> rms_{0.0f};
};

} // namespace incdaw::engine
