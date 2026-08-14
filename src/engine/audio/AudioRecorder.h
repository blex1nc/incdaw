#pragma once

#include "engine/audio/AudioCaptureSink.h"
#include "engine/audio/WavFile.h"
#include "engine/core/SampleRingBuffer.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace incdaw::engine {

/// Records captured audio to a WAV file, realtime-safely.
///
/// The audio thread interleaves each block into a lock-free ring and returns;
/// a writer thread drains the ring to disk through WavStreamWriter. The two
/// sides never share a lock, so a slow disk stalls the writer thread, never
/// the callback — the ring absorbs the stall, and if it fills, whole frames
/// are dropped and counted rather than blocking. A damaged take that says so
/// beats a perfect-looking one that caused a dropout.
///
/// Latency compensation happens at the reporting edge, not per sample: the
/// take's start is the first block's capture timestamp minus the input
/// latency the caller passed (AudioDevice::totalInputLatencyFrames). Proving
/// that number is actually applied is the Phase 12 exit criterion, and the
/// loopback test does exactly that.
class AudioRecorder final : public AudioCaptureSink {
public:
    AudioRecorder() = default;

    /// Stops and discards nothing: if still recording, the take is drained
    /// and finalized so destruction never loses audio already captured.
    ~AudioRecorder() override;

    AudioRecorder(const AudioRecorder&)            = delete;
    AudioRecorder& operator=(const AudioRecorder&) = delete;

    struct Options {
        SampleRate      sampleRate   = 48000.0;
        std::size_t     channelCount = 2;

        /// Largest block the device may deliver; sizes the interleave scratch.
        /// Must come from AudioEngine::maxServiceableBlockSize.
        FrameCount      maxBlockSize = 4096;

        /// Total input latency to subtract when reporting the take's start.
        /// From AudioDevice::totalInputLatencyFrames; zero disables
        /// compensation (the loopback test asserts that this misaligns).
        FrameCount      latencyFrames = 0;

        /// How much audio the ring absorbs while the disk stalls.
        double          ringSeconds = 4.0;

        WavFile::Format format = WavFile::Format::float32;
    };

    /// The finished take, reported by stop().
    struct Take {
        bool        succeeded = false;
        std::string error;

        std::filesystem::path path;
        FrameCount            frameCount = 0;

        /// Capture time of the take's first frame, input latency already
        /// subtracted — where this audio actually belongs on the host clock.
        std::uint64_t startHostTimeNanos = 0;

        /// Frames the ring could not absorb. Nonzero means the take has gaps
        /// and everything after the first drop sits earlier than it should;
        /// the UI must surface it, not average it away.
        std::uint64_t droppedFrames = 0;

        explicit operator bool() const noexcept { return succeeded; }
    };

    /// Allocates everything, opens the file, starts the writer thread. Only
    /// after all of that does the capture callback see the recording flag.
    [[nodiscard]] WavFile::Result start(const std::filesystem::path& path, const Options& options);

    /// Stops accepting audio, drains the ring to disk, finalizes the file.
    Take stop();

    [[nodiscard]] bool isRecording() const noexcept
    {
        return state_.load(std::memory_order_acquire) == State::recording;
    }

    /// Frames accepted into the ring so far (not yet necessarily on disk).
    [[nodiscard]] FrameCount    capturedFrames() const noexcept { return capturedFrames_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t droppedFrames()  const noexcept { return droppedFrames_.load(std::memory_order_relaxed); }

    /// The realtime side. Wait-free; drops and counts when the ring is full.
    void captureAudioBlock(const float* const* inputChannels, std::size_t channelCount,
                           FrameCount frameCount, std::uint64_t blockHostTimeNanos) noexcept override;

private:
    enum class State : int { idle, recording, draining };

    std::atomic<State> state_{State::idle};

    Options               options_;
    std::filesystem::path path_;

    SampleRingBuffer    ring_;
    std::vector<Sample> interleaveScratch_;   ///< maxBlockSize * channels, preallocated
    std::thread         writerThread_;

    std::atomic<std::uint64_t> firstBlockHostTimeNanos_{0};
    std::atomic<FrameCount>    capturedFrames_{0};
    std::atomic<std::uint64_t> droppedFrames_{0};

    // Written only by the writer thread while it runs, read after join.
    FrameCount  framesOnDisk_ = 0;
    std::string writerError_;
};

} // namespace incdaw::engine
