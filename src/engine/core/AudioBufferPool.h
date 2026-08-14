#pragma once

#include "engine/core/AudioBuffer.h"

#include <memory>
#include <vector>

namespace incdaw::engine {

/// Owns the sample memory the audio thread reads and writes.
///
/// Allocated once when a graph is compiled, on a non-realtime thread, and never
/// resized while the audio thread can see it (docs/AUDIO_ENGINE.md §4). The
/// audio thread only ever receives `AudioBufferView`s into this storage.
///
/// All channels of all buffers live in one allocation. Beyond avoiding
/// allocator churn, this keeps a graph's working set contiguous, which matters
/// more than it looks: a mixer touches every buffer once per block, and
/// scattered allocations turn that into a cache-miss tour.
class AudioBufferPool {
public:
    AudioBufferPool() = default;

    /// Allocates `bufferCount` buffers, each `channelCount` x `frameCount`.
    /// Must not be called while the audio thread is using this pool.
    void allocate(std::size_t bufferCount, std::size_t channelCount, FrameCount frameCount);

    [[nodiscard]] std::size_t bufferCount()  const noexcept { return bufferCount_; }
    [[nodiscard]] std::size_t channelCount() const noexcept { return channelCount_; }
    [[nodiscard]] FrameCount  frameCount()   const noexcept { return frameCount_; }

    /// Realtime-safe: pointer arithmetic only.
    [[nodiscard]] AudioBufferView buffer(std::size_t index) const noexcept
    {
        if (index >= bufferCount_)
            return {};

        return AudioBufferView{channelPointers_.data() + index * channelCount_,
                               channelCount_,
                               frameCount_};
    }

    /// Silences every buffer. Called once per block before the graph runs, so
    /// that a node which fails to write leaves silence rather than the previous
    /// block's audio.
    void clearAll() const noexcept
    {
        for (std::size_t index = 0; index < bufferCount_; ++index)
            buffer(index).clear();
    }

    void reset() noexcept;

private:
    std::unique_ptr<Sample[]> samples_;
    std::vector<Sample*>      channelPointers_;
    std::size_t               bufferCount_  = 0;
    std::size_t               channelCount_ = 0;
    FrameCount                frameCount_   = 0;
};

} // namespace incdaw::engine
