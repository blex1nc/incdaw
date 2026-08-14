#include "engine/audio/AudioLogger.h"

#include <algorithm>
#include <cstring>

namespace incdaw::engine {

void AudioLogger::prepare(SampleRate sampleRate, std::size_t channelCount, double seconds)
{
    ready_.store(false, std::memory_order_release);

    sampleRate_     = sampleRate;
    channelCount_   = channelCount > 0 ? channelCount : 1;
    capacityFrames_ = static_cast<FrameCount>(sampleRate * seconds);

    if (capacityFrames_ <= 0)
        return;

    circle_.assign(static_cast<std::size_t>(capacityFrames_) * channelCount_, 0.0f);

    // Sized generously: the largest block any device has handed us so far is
    // well under this, and an oversized block is truncated rather than lost.
    scratch_.assign(static_cast<std::size_t>(8192) * channelCount_, 0.0f);

    written_.store(0, std::memory_order_release);
    ready_.store(true, std::memory_order_release);
}

void AudioLogger::log(const float* const* channels, std::size_t channelCount,
                      FrameCount frameCount) noexcept
{
    if (!enabled_.load(std::memory_order_acquire) || !ready_.load(std::memory_order_acquire)
        || frameCount <= 0 || channelCount == 0)
        return;

    FrameCount frames = frameCount;
    if (static_cast<std::size_t>(frames) * channelCount_ > scratch_.size())
        frames = static_cast<FrameCount>(scratch_.size() / channelCount_);

    // Interleave; channels the caller did not provide log as silence.
    for (FrameCount frame = 0; frame < frames; ++frame)
        for (std::size_t channel = 0; channel < channelCount_; ++channel)
            scratch_[static_cast<std::size_t>(frame) * channelCount_ + channel] =
                channel < channelCount ? channels[channel][frame] : 0.0f;

    // Into the circle, wrapping once at most (frames << capacity).
    const std::uint64_t start = written_.load(std::memory_order_relaxed);
    const auto position = static_cast<FrameCount>(
        start % static_cast<std::uint64_t>(capacityFrames_));

    const FrameCount untilEnd = capacityFrames_ - position;
    const FrameCount first    = frames < untilEnd ? frames : untilEnd;

    std::memcpy(circle_.data() + static_cast<std::size_t>(position) * channelCount_,
                scratch_.data(),
                static_cast<std::size_t>(first) * channelCount_ * sizeof(Sample));

    if (frames > first)
        std::memcpy(circle_.data(),
                    scratch_.data() + static_cast<std::size_t>(first) * channelCount_,
                    static_cast<std::size_t>(frames - first) * channelCount_ * sizeof(Sample));

    written_.store(start + static_cast<std::uint64_t>(frames), std::memory_order_release);
}

FrameCount AudioLogger::grab(AudioFileData& out) const
{
    out.sampleRate   = sampleRate_;
    out.channelCount = channelCount_;
    out.frameCount   = 0;
    out.channels.assign(channelCount_, {});

    if (!ready_.load(std::memory_order_acquire))
        return 0;

    const std::uint64_t before = written_.load(std::memory_order_acquire);
    if (before == 0)
        return 0;

    const auto available = static_cast<FrameCount>(
        std::min<std::uint64_t>(before, static_cast<std::uint64_t>(capacityFrames_)));

    // Copy the window oldest-first, then find out how much of the oldest end
    // the writer overran during the copy and trim exactly that. The result
    // is a slightly shorter grab, never a torn one.
    std::vector<Sample> window(static_cast<std::size_t>(available) * channelCount_);

    const auto oldestPosition = static_cast<FrameCount>(
        (before - static_cast<std::uint64_t>(available))
        % static_cast<std::uint64_t>(capacityFrames_));

    const FrameCount untilEnd = capacityFrames_ - oldestPosition;
    const FrameCount first    = available < untilEnd ? available : untilEnd;

    std::memcpy(window.data(),
                circle_.data() + static_cast<std::size_t>(oldestPosition) * channelCount_,
                static_cast<std::size_t>(first) * channelCount_ * sizeof(Sample));

    if (available > first)
        std::memcpy(window.data() + static_cast<std::size_t>(first) * channelCount_,
                    circle_.data(),
                    static_cast<std::size_t>(available - first) * channelCount_ * sizeof(Sample));

    const std::uint64_t after   = written_.load(std::memory_order_acquire);
    const auto          overrun = static_cast<FrameCount>(std::min<std::uint64_t>(
        after - before, static_cast<std::uint64_t>(available)));

    const FrameCount frames = available - overrun;
    if (frames <= 0)
        return 0;

    out.frameCount = frames;
    for (std::size_t channel = 0; channel < channelCount_; ++channel) {
        out.channels[channel].resize(static_cast<std::size_t>(frames));

        for (FrameCount frame = 0; frame < frames; ++frame)
            out.channels[channel][static_cast<std::size_t>(frame)] =
                window[static_cast<std::size_t>(frame + overrun) * channelCount_ + channel];
    }

    return frames;
}

} // namespace incdaw::engine
