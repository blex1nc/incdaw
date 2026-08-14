#include "engine/core/AudioBufferPool.h"

namespace incdaw::engine {

void AudioBufferPool::allocate(std::size_t bufferCount, std::size_t channelCount, FrameCount frameCount)
{
    if (bufferCount == 0 || channelCount == 0 || frameCount <= 0) {
        reset();
        return;
    }

    const std::size_t frames        = static_cast<std::size_t>(frameCount);
    const std::size_t totalChannels = bufferCount * channelCount;
    const std::size_t totalSamples  = totalChannels * frames;

    samples_ = std::make_unique<Sample[]>(totalSamples);

    channelPointers_.clear();
    channelPointers_.reserve(totalChannels);

    for (std::size_t channel = 0; channel < totalChannels; ++channel)
        channelPointers_.push_back(samples_.get() + channel * frames);

    bufferCount_  = bufferCount;
    channelCount_ = channelCount;
    frameCount_   = frameCount;

    clearAll();
}

void AudioBufferPool::reset() noexcept
{
    samples_.reset();
    channelPointers_.clear();
    bufferCount_  = 0;
    channelCount_ = 0;
    frameCount_   = 0;
}

} // namespace incdaw::engine
