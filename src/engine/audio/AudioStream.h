#pragma once

#include "engine/audio/WavStreamReader.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace incdaw::engine {

/// One streamed audio source: a windowed view of a file on disk.
///
/// Two segments leapfrog ahead of the play position — while the audio thread
/// reads one, the service thread refills the other past it. Each segment is
/// guarded by its own seqlock: the audio thread never waits, and a read torn
/// by a concurrent refill is served as silence and counted, exactly like a
/// ring overflow in the recorder. An underrun here is audible and honest;
/// a blocked audio thread would be a dropout for every track at once.
///
/// The audio thread's `read` also publishes the position it wanted, which is
/// all the steering the service thread needs: seeks are not special, they are
/// just a position the window does not cover yet.
class AudioStream {
public:
    /// Opens the file and sizes both segments. Smaller segments are for tests
    /// (they force refills); the default holds ~1.4 s of 48 kHz audio each.
    [[nodiscard]] WavFile::Result open(const std::filesystem::path& path,
                                       FrameCount segmentFrames = 65536);

    [[nodiscard]] SampleRate  sampleRate()   const noexcept { return reader_.sampleRate(); }
    [[nodiscard]] std::size_t channelCount() const noexcept { return reader_.channelCount(); }
    [[nodiscard]] FrameCount  frameCount()   const noexcept { return reader_.frameCount(); }

    /// Synchronously fills the window at `sourceFrame`. Called at graph
    /// compile time so a rebuilt graph starts warm rather than serving an
    /// underrun on its first block.
    void prefill(FrameCount sourceFrame);

    /// The realtime side. Copies `frames` samples of `channel` starting at
    /// source frame `firstFrame` into `dest`; whatever the window cannot
    /// serve is zero-filled and counted. Wait-free.
    void read(FrameCount firstFrame, FrameCount frames, std::size_t channel,
              Sample* dest) noexcept;

    [[nodiscard]] std::uint64_t underrunCount() const noexcept
    {
        return underruns_.load(std::memory_order_relaxed);
    }

    /// Steers the window toward the last requested position, refilling at
    /// most one segment per call. Service-thread (or test) side only.
    void service();

private:
    struct Segment {
        /// Seqlock: odd while the service thread rewrites `data`.
        std::atomic<std::uint64_t> version{0};

        /// Source frame of data[0]; -1 while the segment holds nothing.
        std::atomic<FrameCount> start{-1};

        std::vector<Sample>  data;       ///< planar: channelCount * segmentFrames
        std::vector<Sample*> channels;   ///< service-side pointers into data
    };

    void fillSegment(Segment& segment, FrameCount sourceFrame);

    Segment         segments_[2];
    WavStreamReader reader_;
    FrameCount      segmentFrames_ = 0;

    std::atomic<FrameCount>    lastRequested_{0};
    std::atomic<std::uint64_t> underruns_{0};
};

/// Services every live AudioStream from one background thread.
///
/// Streams are held weakly: a stream dies with its graph (off the audio
/// thread, via the engine's reaper) and simply stops being serviced. The
/// polling loop is the recorder's writer-thread pattern — a few milliseconds
/// of latency against a multi-second window, and not even a wait-free
/// syscall anywhere near the audio thread.
class DiskStreamer {
public:
    DiskStreamer();
    ~DiskStreamer();

    DiskStreamer(const DiskStreamer&)            = delete;
    DiskStreamer& operator=(const DiskStreamer&) = delete;

    void add(std::shared_ptr<AudioStream> stream);

    /// One pass over every live stream. The thread body, exposed so tests
    /// drive servicing deterministically instead of sleeping and hoping.
    void serviceOnce();

private:
    std::mutex                              mutex_;
    std::vector<std::weak_ptr<AudioStream>> streams_;

    std::thread       thread_;
    std::atomic<bool> running_{true};
};

} // namespace incdaw::engine
