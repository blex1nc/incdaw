#include "engine/transport/Transport.h"

namespace incdaw::engine {

Transport::Transport(TempoMap map) : tempoMap_(std::move(map)) {}

void Transport::play() noexcept
{
    state_.store(TransportState::playing, std::memory_order_release);
}

void Transport::stop() noexcept
{
    state_.store(TransportState::stopped, std::memory_order_release);

    // Stop returns to where playback began, which for now is the loop start when
    // looping and the song start otherwise. Pause is the operation that holds
    // position.
    seek(loopEnabled_.load(std::memory_order_relaxed) ? loopStart_.load(std::memory_order_relaxed) : 0);
}

void Transport::pause() noexcept
{
    state_.store(TransportState::stopped, std::memory_order_release);
}

void Transport::startRecording() noexcept
{
    state_.store(TransportState::recording, std::memory_order_release);
}

void Transport::seek(FramePosition frame) noexcept
{
    seekTarget_.store(frame < 0 ? 0 : frame, std::memory_order_relaxed);
    seekRequested_.store(true, std::memory_order_release);
}

void Transport::setLoopRange(FramePosition start, FramePosition end) noexcept
{
    if (start < 0)
        start = 0;

    // An inverted or empty range would make the wrap logic loop forever inside
    // one block; reject it rather than clamping to something the user did not ask
    // for.
    if (end <= start)
        return;

    loopStart_.store(start, std::memory_order_relaxed);
    loopEnd_.store(end, std::memory_order_relaxed);
}

void Transport::setTempoMap(TempoMap map)
{
    tempoMap_ = std::move(map);
}

void Transport::applyPendingSeek() noexcept
{
    if (seekRequested_.exchange(false, std::memory_order_acquire))
        position_.store(seekTarget_.load(std::memory_order_relaxed), std::memory_order_release);
}

std::size_t Transport::processBlock(FrameCount blockSize, BlockSegment* segments,
                                    std::size_t maxSegments) noexcept
{
    if (segments == nullptr || maxSegments == 0 || blockSize <= 0)
        return 0;

    applyPendingSeek();

    FramePosition position = position_.load(std::memory_order_relaxed);

    if (state_.load(std::memory_order_acquire) == TransportState::stopped) {
        // Not advancing, but still one segment: a synth releasing a note must
        // keep being rendered after the transport stops.
        segments[0] = BlockSegment{0, blockSize, position, false};
        return 1;
    }

    const bool          looping   = loopEnabled_.load(std::memory_order_relaxed);
    const FramePosition loopBegin = loopStart_.load(std::memory_order_relaxed);
    const FramePosition loopFinish = loopEnd_.load(std::memory_order_relaxed);
    const bool          loopValid = looping && loopFinish > loopBegin;

    // A seek can land outside the loop; playback continues from there rather
    // than being yanked back, which is what the user asked for.
    std::size_t count     = 0;
    FrameCount  remaining = blockSize;
    FrameCount  offset    = 0;
    bool        wrapped   = false;

    while (remaining > 0 && count < maxSegments) {
        FrameCount length = remaining;

        if (loopValid && position >= loopBegin && position < loopFinish) {
            const FrameCount untilLoopEnd = loopFinish - position;
            if (untilLoopEnd < length)
                length = untilLoopEnd;
        }

        segments[count] = BlockSegment{offset, length, position, wrapped};
        ++count;

        position  += length;
        offset    += length;
        remaining -= length;

        wrapped = false;

        // Wrap only if this segment actually started inside the loop. A
        // position past the loop end — reached by seeking beyond it — must play
        // on from there, not be yanked backwards into a loop the user left.
        const FramePosition segmentStart = segments[count - 1].startFrame;
        const bool startedInsideLoop = segmentStart >= loopBegin && segmentStart < loopFinish;

        if (loopValid && startedInsideLoop && position >= loopFinish) {
            position = loopBegin;
            wrapped  = true;
        }
    }

    // If the plan ran out of segments (a loop shorter than one block, repeated
    // many times), the remainder is folded into the last segment rather than
    // dropped. The timing is then wrong for that pathological case, but audio
    // keeps flowing — silence would be worse and harder to diagnose.
    if (remaining > 0 && count > 0)
        segments[count - 1].length += remaining;

    position_.store(position, std::memory_order_release);
    return count;
}

} // namespace incdaw::engine
