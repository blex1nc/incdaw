#include "engine/audio/AudioStream.h"

#include <chrono>
#include <cstring>

namespace incdaw::engine {

// ── AudioStream ──────────────────────────────────────────────────────────────

WavFile::Result AudioStream::open(const std::filesystem::path& path, FrameCount segmentFrames)
{
    WavFile::Result result = reader_.open(path);
    if (!result)
        return result;

    segmentFrames_ = segmentFrames > 0 ? segmentFrames : 1;

    for (Segment& segment : segments_) {
        segment.data.assign(reader_.channelCount() * static_cast<std::size_t>(segmentFrames_),
                            0.0f);
        segment.channels.resize(reader_.channelCount());

        for (std::size_t channel = 0; channel < reader_.channelCount(); ++channel)
            segment.channels[channel] =
                segment.data.data() + channel * static_cast<std::size_t>(segmentFrames_);

        segment.start.store(-1, std::memory_order_release);
        segment.version.store(0, std::memory_order_release);
    }

    return result;
}

void AudioStream::prefill(FrameCount sourceFrame)
{
    const FrameCount from = sourceFrame > 0 ? sourceFrame : 0;

    lastRequested_.store(from, std::memory_order_relaxed);
    fillSegment(segments_[0], from);
    fillSegment(segments_[1], from + segmentFrames_);
}

void AudioStream::fillSegment(Segment& segment, FrameCount sourceFrame)
{
    // Seqlock write: odd while the data is inconsistent. The audio thread
    // sees the odd version (or a changed one) and serves silence instead.
    segment.version.fetch_add(1, std::memory_order_release);
    segment.start.store(-1, std::memory_order_release);

    (void)reader_.readAt(sourceFrame, segmentFrames_, segment.channels.data(),
                         reader_.channelCount());

    segment.start.store(sourceFrame, std::memory_order_release);
    segment.version.fetch_add(1, std::memory_order_release);
}

void AudioStream::read(FrameCount firstFrame, FrameCount frames, std::size_t channel,
                       Sample* dest) noexcept
{
    lastRequested_.store(firstFrame, std::memory_order_relaxed);

    if (frames <= 0)
        return;

    if (channel >= reader_.channelCount())
        channel = reader_.channelCount() - 1;

    std::memset(dest, 0, static_cast<std::size_t>(frames) * sizeof(Sample));

    FrameCount position = firstFrame;
    const FrameCount end = firstFrame + frames;
    bool missed = false;

    // A block is at most a few thousand frames against multi-thousand-frame
    // segments, so it spans at most two of them; four passes is a hard bound,
    // not a hope.
    for (int pass = 0; pass < 4 && position < end; ++pass) {
        Segment* covering = nullptr;
        FrameCount coveringStart = 0;

        for (Segment& segment : segments_) {
            const FrameCount start = segment.start.load(std::memory_order_acquire);
            if (start >= 0 && position >= start && position < start + segmentFrames_) {
                covering      = &segment;
                coveringStart = start;
                break;
            }
        }

        if (covering == nullptr) {
            missed = true;
            break;   // the rest stays the silence memset wrote
        }

        const FrameCount available = coveringStart + segmentFrames_ - position;
        const FrameCount todo      = available < end - position ? available : end - position;

        const std::uint64_t before = covering->version.load(std::memory_order_acquire);
        if (before & 1u) {
            missed = true;
            break;   // mid-refill; silence is already in place
        }

        const Sample* source = covering->data.data()
                             + channel * static_cast<std::size_t>(segmentFrames_)
                             + static_cast<std::size_t>(position - coveringStart);

        std::memcpy(dest + (position - firstFrame), source,
                    static_cast<std::size_t>(todo) * sizeof(Sample));

        if (covering->version.load(std::memory_order_acquire) != before) {
            // Torn by a concurrent refill: what was copied cannot be trusted.
            std::memset(dest + (position - firstFrame), 0,
                        static_cast<std::size_t>(todo) * sizeof(Sample));
            missed = true;
            break;
        }

        position += todo;
    }

    if (missed || position < end)
        underruns_.fetch_add(1, std::memory_order_relaxed);
}

void AudioStream::service()
{
    const FrameCount desired = lastRequested_.load(std::memory_order_relaxed);

    // Which segment covers the position being played?
    Segment* current = nullptr;
    FrameCount currentStart = 0;

    for (Segment& segment : segments_) {
        const FrameCount start = segment.start.load(std::memory_order_acquire);
        if (start >= 0 && desired >= start && desired < start + segmentFrames_) {
            current      = &segment;
            currentStart = start;
            break;
        }
    }

    if (current == nullptr) {
        // The window is somewhere else entirely (a seek, or first use without
        // prefill). Move it wholesale.
        fillSegment(segments_[0], desired);
        fillSegment(segments_[1], desired + segmentFrames_);
        return;
    }

    // Leapfrog: the other segment must hold what comes next.
    Segment& other = &segments_[0] == current ? segments_[1] : segments_[0];
    const FrameCount next = currentStart + segmentFrames_;

    if (other.start.load(std::memory_order_acquire) != next)
        fillSegment(other, next);
}

// ── DiskStreamer ─────────────────────────────────────────────────────────────

DiskStreamer::DiskStreamer()
{
    thread_ = std::thread([this] {
        while (running_.load(std::memory_order_acquire)) {
            serviceOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    });
}

DiskStreamer::~DiskStreamer()
{
    running_.store(false, std::memory_order_release);

    if (thread_.joinable())
        thread_.join();
}

void DiskStreamer::add(std::shared_ptr<AudioStream> stream)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    streams_.push_back(std::move(stream));
}

void DiskStreamer::serviceOnce()
{
    // Pin the live streams outside the lock: servicing does file I/O, and
    // holding the mutex through it would stall add() from the UI thread.
    std::vector<std::shared_ptr<AudioStream>> live;

    {
        const std::lock_guard<std::mutex> lock(mutex_);

        for (auto entry = streams_.begin(); entry != streams_.end();) {
            if (auto stream = entry->lock()) {
                live.push_back(std::move(stream));
                ++entry;
            } else {
                entry = streams_.erase(entry);   // its graph is gone
            }
        }
    }

    for (const auto& stream : live)
        stream->service();
}

} // namespace incdaw::engine
