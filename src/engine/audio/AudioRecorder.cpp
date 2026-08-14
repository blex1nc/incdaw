#include "engine/audio/AudioRecorder.h"

#include "engine/audio/WavStreamWriter.h"

#include <chrono>

namespace incdaw::engine {
namespace {

/// How long the writer thread sleeps when the ring is empty. Small enough
/// that the ring never comes close to filling at any realistic block size,
/// large enough that an idle recording costs nothing measurable. Polling
/// rather than a semaphore keeps the capture path free of even a wait-free
/// syscall.
constexpr auto writerIdleSleep = std::chrono::milliseconds(2);

/// Samples the writer moves per drain, chosen so one drain is one comfortable
/// write(2) rather than thousands of tiny ones.
constexpr std::size_t writerChunkSamples = 65536;

} // namespace

AudioRecorder::~AudioRecorder()
{
    (void)stop();
}

WavFile::Result AudioRecorder::start(const std::filesystem::path& path, const Options& options)
{
    WavFile::Result result;

    if (state_.load(std::memory_order_acquire) != State::idle) {
        result.error = "already recording";
        return result;
    }

    if (options.channelCount == 0 || options.sampleRate <= 0.0 || options.maxBlockSize <= 0) {
        result.error = "invalid recorder options";
        return result;
    }

    options_ = options;
    path_    = path;

    const auto ringSamples = static_cast<std::size_t>(
        options.sampleRate * options.ringSeconds * static_cast<double>(options.channelCount));

    // Everything the capture callback will touch is allocated here, before the
    // recording flag is raised. The callback itself never allocates.
    ring_.reset(ringSamples);
    interleaveScratch_.assign(
        static_cast<std::size_t>(options.maxBlockSize) * options.channelCount, 0.0f);

    firstBlockHostTimeNanos_.store(0, std::memory_order_relaxed);
    capturedFrames_.store(0, std::memory_order_relaxed);
    droppedFrames_.store(0, std::memory_order_relaxed);
    framesOnDisk_ = 0;
    writerError_.clear();

    // The writer opens the file so that a failure to create it is discovered
    // here, synchronously, not one poll later on a detached thread.
    auto writer = std::make_unique<WavStreamWriter>();
    result = writer->open(path, options.sampleRate, options.channelCount, options.format);
    if (!result)
        return result;

    writerThread_ = std::thread([this, writer = std::move(writer)]() mutable {
        // The chunk is a whole number of frames so a drain can never split one.
        std::vector<Sample> chunk(writerChunkSamples - writerChunkSamples % options_.channelCount);

        while (true) {
            const std::size_t got = ring_.read(chunk.data(), chunk.size());

            if (got > 0) {
                if (writerError_.empty()) {
                    const auto appended = writer->appendInterleaved(
                        chunk.data(), static_cast<FrameCount>(got / options_.channelCount));
                    if (!appended)
                        writerError_ = appended.error;   // keep draining; report at stop
                }
            } else if (state_.load(std::memory_order_acquire) == State::draining) {
                break;   // producer has stopped and the ring is empty
            } else {
                std::this_thread::sleep_for(writerIdleSleep);
            }
        }

        framesOnDisk_ = writer->frameCount();

        const auto finalized = writer->finalize();
        if (writerError_.empty() && !finalized)
            writerError_ = finalized.error;
    });

    state_.store(State::recording, std::memory_order_release);

    result.succeeded = true;
    return result;
}

AudioRecorder::Take AudioRecorder::stop()
{
    Take take;

    if (state_.load(std::memory_order_acquire) == State::idle) {
        take.error = "not recording";
        return take;
    }

    // The capture callback sees `draining` and stops feeding the ring; the
    // writer thread sees it, drains what remains, finalizes and exits.
    state_.store(State::draining, std::memory_order_release);

    if (writerThread_.joinable())
        writerThread_.join();

    take.path          = path_;
    take.frameCount    = framesOnDisk_;
    take.droppedFrames = droppedFrames_.load(std::memory_order_relaxed);

    // The compensation itself: the first block's capture timestamp, pulled
    // back by the latency the device reported. This subtraction is what the
    // loopback exit criterion measures.
    const std::uint64_t firstBlock = firstBlockHostTimeNanos_.load(std::memory_order_relaxed);
    const auto latencyNanos = static_cast<std::uint64_t>(
        framesToSeconds(options_.latencyFrames, options_.sampleRate) * 1.0e9 + 0.5);

    take.startHostTimeNanos = firstBlock > latencyNanos ? firstBlock - latencyNanos : 0;

    if (!writerError_.empty())
        take.error = writerError_;
    else
        take.succeeded = true;

    state_.store(State::idle, std::memory_order_release);
    return take;
}

void AudioRecorder::captureAudioBlock(const float* const* inputChannels, std::size_t channelCount,
                                      FrameCount frameCount, std::uint64_t blockHostTimeNanos) noexcept
{
    if (state_.load(std::memory_order_acquire) != State::recording || frameCount <= 0)
        return;

    // Clamp to what the scratch was sized for. Larger cannot happen when
    // maxBlockSize came from the device's own maximum; if it somehow does,
    // recording the truncation as dropped frames keeps the take honest.
    FrameCount frames = frameCount;
    if (frames > options_.maxBlockSize) {
        droppedFrames_.fetch_add(static_cast<std::uint64_t>(frames - options_.maxBlockSize),
                                 std::memory_order_relaxed);
        frames = options_.maxBlockSize;
    }

    if (firstBlockHostTimeNanos_.load(std::memory_order_relaxed) == 0)
        firstBlockHostTimeNanos_.store(blockHostTimeNanos, std::memory_order_relaxed);

    // Interleave into the preallocated scratch. Channels the device did not
    // deliver record as silence rather than stale scratch contents.
    const std::size_t recordChannels = options_.channelCount;

    for (FrameCount frame = 0; frame < frames; ++frame) {
        for (std::size_t channel = 0; channel < recordChannels; ++channel) {
            interleaveScratch_[static_cast<std::size_t>(frame) * recordChannels + channel]
                = channel < channelCount ? inputChannels[channel][static_cast<std::size_t>(frame)]
                                         : 0.0f;
        }
    }

    // Whole frames only: a partial frame in the ring would shift every later
    // sample by one channel, which is unrecoverable. If the ring cannot take
    // the whole block's worth of remaining frames, the shortfall is dropped
    // and counted.
    const std::size_t wantSamples = static_cast<std::size_t>(frames) * recordChannels;
    const std::size_t fitFrames   = ring_.freeSpace() / recordChannels;
    const std::size_t takeFrames  = static_cast<std::size_t>(frames) < fitFrames
                                        ? static_cast<std::size_t>(frames) : fitFrames;

    const std::size_t wrote = ring_.write(interleaveScratch_.data(), takeFrames * recordChannels);
    (void)wrote;   // freeSpace() is only advanced by this thread, so it all fits
    (void)wantSamples;

    capturedFrames_.fetch_add(static_cast<FrameCount>(takeFrames), std::memory_order_relaxed);

    if (takeFrames < static_cast<std::size_t>(frames))
        droppedFrames_.fetch_add(static_cast<std::uint64_t>(frames) - takeFrames,
                                 std::memory_order_relaxed);
}

} // namespace incdaw::engine
