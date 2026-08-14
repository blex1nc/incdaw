#pragma once

#include "engine/audio/WavFile.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace incdaw::engine {

/// The Audio Logger: the last minute of the master, always.
///
/// A circular buffer the audio thread overwrites forever — keep-newest,
/// where the recorder's ring is keep-oldest — so the take you did not think
/// to record is still there when you reach for it. `grab` copies the window
/// out without stopping the writer: it snapshots the monotonic write count,
/// copies, re-reads the count, and TRIMS whatever the writer overran during
/// the copy. A grab is at worst a few milliseconds short at its far end,
/// never torn in the middle, and never a lock anywhere near the callback.
///
/// This logs the MASTER — what the graph rendered, which includes monitored
/// input only while monitoring is on. An input-side pre-record buffer is a
/// separate (deferred) feature; conflating them would log the microphone
/// while the user believed only playback was kept.
class AudioLogger {
public:
    /// Allocates for `seconds` of `channelCount` audio. Off the audio
    /// thread only; the logger is unready during the call.
    void prepare(SampleRate sampleRate, std::size_t channelCount, double seconds);

    void setEnabled(bool enabled) noexcept { enabled_.store(enabled, std::memory_order_release); }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_.load(std::memory_order_acquire); }

    [[nodiscard]] SampleRate  sampleRate()   const noexcept { return sampleRate_; }
    [[nodiscard]] std::size_t channelCount() const noexcept { return channelCount_; }

    /// The realtime side: interleaves one planar block into the circle.
    /// Wait-free; a no-op while disabled or unprepared.
    void log(const float* const* channels, std::size_t channelCount,
             FrameCount frameCount) noexcept;

    /// Copies the freshest window (up to the prepared length) into `out`,
    /// planar, oldest first. Returns the frames delivered. Off-RT.
    [[nodiscard]] FrameCount grab(AudioFileData& out) const;

private:
    std::vector<Sample> circle_;      ///< interleaved, capacityFrames_ * channelCount_
    std::vector<Sample> scratch_;     ///< RT interleave scratch, one block

    SampleRate  sampleRate_     = 0.0;
    std::size_t channelCount_   = 0;
    FrameCount  capacityFrames_ = 0;

    /// Total frames ever written; the write position is this modulo the
    /// capacity. Monotonic, which is what lets `grab` detect an overrun.
    std::atomic<std::uint64_t> written_{0};

    std::atomic<bool> enabled_{false};
    std::atomic<bool> ready_{false};
};

} // namespace incdaw::engine
