#pragma once

#include "engine/core/AudioBuffer.h"
#include "engine/core/Time.h"

#include <atomic>
#include <cmath>

namespace incdaw::engine {

/// A one-pole ramp towards a target gain.
///
/// Every fader in the application goes through one of these. An instantaneous
/// gain change produces a step in the waveform, which is audible as a click —
/// the single most common artefact in a naive mixer. Five milliseconds is short
/// enough to feel immediate under the hand and long enough to remove the step.
///
/// Lives in `core/` rather than in a node because gain, pan, mute and polarity
/// all need it and none of them should reimplement it.
class Smoother {
public:
    static constexpr double defaultSmoothingSeconds = 0.005;

    void prepare(SampleRate sampleRate, double seconds = defaultSmoothingSeconds) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        seconds_    = seconds > 0.0 ? seconds : defaultSmoothingSeconds;
        coefficient_ = static_cast<Sample>(1.0 - std::exp(-1.0 / (seconds_ * sampleRate_)));
        current_    = target_.load(std::memory_order_relaxed);
    }

    void setTarget(Sample target) noexcept { target_.store(target, std::memory_order_relaxed); }
    [[nodiscard]] Sample target() const noexcept { return target_.load(std::memory_order_relaxed); }

    /// Value reached at the end of the last block; differs from `target()`
    /// while a change is still ramping in.
    [[nodiscard]] Sample current() const noexcept { return current_; }

    void snap() noexcept { current_ = target(); }

    /// Multiplies one channel in place, advancing the ramp. Realtime-safe.
    ///
    /// `commit` decides whether the ramp's state advances: a strip applying the
    /// same ramp to several channels must advance it exactly once, or the
    /// channels would end up at different points in it.
    void applyToChannel(Sample* samples, FrameCount frameCount, bool commit) noexcept
    {
        const Sample destination = target();

        if (current_ == destination) {
            if (destination != Sample{1}) {
                for (FrameCount frame = 0; frame < frameCount; ++frame)
                    samples[frame] *= destination;
            }

            return;
        }

        Sample value = current_;
        for (FrameCount frame = 0; frame < frameCount; ++frame) {
            value += (destination - value) * coefficient_;
            samples[frame] *= value;
        }

        if (!commit)
            return;

        current_ = value;

        // Snap once the remainder is inaudible, so the smoothing path does not
        // stay hot forever chasing the last fraction of a decibel.
        const Sample remaining = current_ > destination ? current_ - destination
                                                        : destination - current_;
        if (remaining < 1e-6f)
            current_ = destination;
    }

    /// Multiplies every channel of a buffer, advancing the ramp once.
    void applyTo(const AudioBufferView& buffer, FrameCount frameCount) noexcept
    {
        const std::size_t channels = buffer.channelCount();

        for (std::size_t channel = 0; channel < channels; ++channel)
            applyToChannel(buffer.channel(channel), frameCount, channel + 1 == channels);
    }

private:
    std::atomic<Sample> target_{Sample{1}};
    Sample              current_     = Sample{1};
    Sample              coefficient_ = 1.0f;
    SampleRate          sampleRate_  = 48000.0;
    double              seconds_     = defaultSmoothingSeconds;
};

} // namespace incdaw::engine
