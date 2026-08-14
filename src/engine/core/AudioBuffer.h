#pragma once

#include "engine/core/Time.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace incdaw::engine {

/// A non-owning view over a block of multichannel audio.
///
/// Deliberately does not own its memory. Every buffer the audio thread touches
/// is allocated once, at graph-compile time, on a non-realtime thread
/// (docs/AUDIO_ENGINE.md §4). A buffer type that could allocate would make that
/// rule impossible to hold.
///
/// Channels are separate contiguous arrays (planar), not interleaved: DSP reads
/// one channel at a time, and planar layout is what vectorises.
///
/// A view carries a frame offset so that splitting a block at an event boundary
/// costs nothing and allocates nothing (docs/AUDIO_ENGINE.md §6).
class AudioBufferView {
public:
    constexpr AudioBufferView() noexcept = default;

    constexpr AudioBufferView(Sample* const* channels,
                              std::size_t    channelCount,
                              FrameCount     frames,
                              FrameCount     offset = 0) noexcept
        : channels_(channels), channelCount_(channelCount), frames_(frames), offset_(offset) {}

    [[nodiscard]] constexpr std::size_t channelCount() const noexcept { return channelCount_; }
    [[nodiscard]] constexpr FrameCount  frameCount()   const noexcept { return frames_; }
    [[nodiscard]] constexpr bool        isEmpty()      const noexcept { return channelCount_ == 0 || frames_ <= 0; }

    [[nodiscard]] Sample* channel(std::size_t index) const noexcept
    {
        assert(index < channelCount_);
        assert(channels_ != nullptr);
        return channels_[index] + offset_;
    }

    /// Fills every channel with silence.
    ///
    /// The most common operation in a graph: a node with nothing to say must
    /// say nothing explicitly, because the buffer it was handed still holds
    /// whatever the previous block left in it.
    void clear() const noexcept
    {
        for (std::size_t index = 0; index < channelCount_; ++index) {
            Sample* samples = channel(index);
            for (FrameCount frame = 0; frame < frames_; ++frame)
                samples[frame] = Sample{0};
        }
    }

    /// Adds `source` into this buffer — the mixer's fundamental operation.
    ///
    /// Channel and frame counts need not match: the overlap is processed, the
    /// remainder is left alone. A mismatch is a routing fact, not an error, and
    /// must not read or write out of bounds.
    void addFrom(const AudioBufferView& source) const noexcept
    {
        const std::size_t channels = source.channelCount_ < channelCount_ ? source.channelCount_ : channelCount_;
        const FrameCount  frames   = source.frames_ < frames_ ? source.frames_ : frames_;

        for (std::size_t index = 0; index < channels; ++index) {
            Sample*       destination = channel(index);
            const Sample* input       = source.channel(index);

            for (FrameCount frame = 0; frame < frames; ++frame)
                destination[frame] += input[frame];
        }
    }

    void copyFrom(const AudioBufferView& source) const noexcept
    {
        clear();
        addFrom(source);
    }

    void applyGain(Sample gain) const noexcept
    {
        for (std::size_t index = 0; index < channelCount_; ++index) {
            Sample* samples = channel(index);
            for (FrameCount frame = 0; frame < frames_; ++frame)
                samples[frame] *= gain;
        }
    }

    /// A view over `frames` frames starting `offset` frames into this one.
    ///
    /// Used to split a block at an event boundary. Costs nothing: the sub-view
    /// shares the same channel pointer array and simply carries a larger offset.
    [[nodiscard]] constexpr AudioBufferView subBlock(FrameCount offset, FrameCount frames) const noexcept
    {
        assert(offset >= 0 && frames >= 0 && offset + frames <= frames_);
        return AudioBufferView{channels_, channelCount_, frames, offset_ + offset};
    }

    /// Largest absolute sample value in the buffer. Used by metering and by the
    /// tests that assert a node produced (or did not produce) signal.
    [[nodiscard]] Sample peak() const noexcept
    {
        Sample highest = Sample{0};

        for (std::size_t index = 0; index < channelCount_; ++index) {
            const Sample* samples = channel(index);
            for (FrameCount frame = 0; frame < frames_; ++frame) {
                const Sample magnitude = samples[frame] < Sample{0} ? -samples[frame] : samples[frame];
                if (magnitude > highest)
                    highest = magnitude;
            }
        }

        return highest;
    }

    /// True if any sample is NaN or infinite.
    ///
    /// One misbehaving plugin can poison an entire mix bus with a single NaN,
    /// and the symptom (sudden silence) looks nothing like the cause. The mixer
    /// checks for this at bus boundaries rather than letting it propagate.
    [[nodiscard]] bool hasNonFiniteSamples() const noexcept
    {
        for (std::size_t index = 0; index < channelCount_; ++index) {
            const Sample* samples = channel(index);
            for (FrameCount frame = 0; frame < frames_; ++frame) {
                if (!std::isfinite(samples[frame]))
                    return true;
            }
        }

        return false;
    }

private:
    Sample* const* channels_     = nullptr;
    std::size_t    channelCount_ = 0;
    FrameCount     frames_       = 0;
    FrameCount     offset_       = 0;
};

} // namespace incdaw::engine
