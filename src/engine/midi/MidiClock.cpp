#include "engine/midi/MidiClock.h"

#include "engine/transport/TempoMap.h"

#include <algorithm>

namespace incdaw::engine {
namespace {

constexpr std::uint8_t statusClock    = 0xF8;
constexpr std::uint8_t statusStart    = 0xFA;
constexpr std::uint8_t statusContinue = 0xFB;
constexpr std::uint8_t statusStop     = 0xFC;
constexpr std::uint8_t statusSongPosition = 0xF2;

MidiMessage realtimeMessage(std::uint8_t status, FrameCount offset) noexcept
{
    MidiMessage message;
    message.frameOffset = offset;
    message.status      = status;
    return message;
}

} // namespace

void MidiClockGenerator::reset() noexcept
{
    wasPlaying_         = false;
    hasHistory_         = false;
    expectedFrame_      = 0;
    nextPulse_          = 0;
    stoppedPhaseFrames_ = 0.0;
    pulses_.store(0, std::memory_order_relaxed);
}

void MidiClockGenerator::emitPositionBracket(MidiBuffer& destination, Tick tick,
                                            FrameCount offset) const noexcept
{
    // Song position pointer is defined only while stopped, so a receiver is
    // entitled to ignore one that arrives mid-flight. Stop, tell it where we
    // are, continue.
    //
    // All three share a frame offset, and the buffer's insertion sort is
    // stable, so they stay in this order rather than being reshuffled among
    // the pulses that land on the same frame.
    (void)destination.insert(realtimeMessage(statusStop, offset));

    const std::int64_t beats = std::clamp<std::int64_t>(tick / ticksPerSongPositionBeat, 0, 16383);

    MidiMessage position;
    position.frameOffset = offset;
    position.status      = statusSongPosition;
    position.data1       = static_cast<std::uint8_t>(beats & 0x7F);
    position.data2       = static_cast<std::uint8_t>((beats >> 7) & 0x7F);
    (void)destination.insert(position);

    (void)destination.insert(realtimeMessage(statusContinue, offset));
}

void MidiClockGenerator::generate(MidiBuffer& destination, const BlockSegment* segments,
                                  std::size_t segmentCount, const TempoMap& tempoMap,
                                  bool playing, FrameCount blockSize) noexcept
{
    if (role_.load(std::memory_order_relaxed) != MidiClockRole::send || blockSize <= 0)
        return;

    const SampleRate rate = tempoMap.sampleRate();
    if (rate <= 0.0)
        return;

    const bool          hasPlan    = segmentCount > 0 && segments != nullptr;
    const FramePosition blockStart = hasPlan ? segments[0].startFrame : 0;

    // ── Transport transitions ────────────────────────────────────────────────

    if (playing && !wasPlaying_) {
        const Tick startTick = tempoMap.tickForFrame(blockStart);

        if (blockStart == 0) {
            // Start means "from the beginning" and carries no position with it.
            (void)destination.insert(realtimeMessage(statusStart, 0));
        } else {
            emitPositionBracket(destination, startTick, 0);
        }

        // Phase is taken from the timeline, not carried over from the
        // free-running clock: from here the pulses belong to the song.
        nextPulse_ = (startTick + ticksPerPulse - 1) / ticksPerPulse;
        stoppedPhaseFrames_ = 0.0;
    } else if (!playing && wasPlaying_) {
        (void)destination.insert(realtimeMessage(statusStop, 0));
        stoppedPhaseFrames_ = 0.0;
    } else if (playing && hasHistory_ && blockStart != expectedFrame_
               && !(hasPlan && segments[0].startsAfterLoopWrap)) {
        // A seek: the block did not begin where the last one ended, and it was
        // not the loop that moved it. A receiver that is not told simply plays
        // on from where it was, and the two drift apart by the size of the
        // jump. (A wrap is handled below, on the segment it happens at, which
        // is where the frame offset actually is.)
        const Tick startTick = tempoMap.tickForFrame(blockStart);
        emitPositionBracket(destination, startTick, 0);
        nextPulse_ = (startTick + ticksPerPulse - 1) / ticksPerPulse;
    }

    // ── The pulses ───────────────────────────────────────────────────────────

    if (playing && hasPlan) {
        for (std::size_t index = 0; index < segmentCount; ++index) {
            const BlockSegment& segment = segments[index];
            if (segment.length <= 0)
                continue;

            // The loop wrapped inside this block. The jump is mid-block, so
            // the bracket goes on the frame it happened at rather than at the
            // block boundary — the receiver's idea of "now" should move at the
            // same instant INCDAW's does.
            if (segment.startsAfterLoopWrap) {
                const Tick wrapTick = tempoMap.tickForFrame(segment.startFrame);
                emitPositionBracket(destination, wrapTick, segment.offset);
                nextPulse_ = (wrapTick + ticksPerPulse - 1) / ticksPerPulse;
            }

            const FramePosition segmentEnd = segment.startFrame + segment.length;

            for (int guard = 0; guard < maxPulsesPerBlock; ++guard) {
                const Tick          tick  = nextPulse_ * ticksPerPulse;
                const FramePosition frame = tempoMap.frameForTick(tick);

                if (frame >= segmentEnd)
                    break;

                // A pulse whose frame rounds just before this segment is late
                // rather than lost: the earliest audible moment is the best
                // answer available, which is what the input path does with a
                // message that arrives behind its own timestamp.
                const FrameCount within = std::max<FrameCount>(frame - segment.startFrame, 0);

                (void)destination.insert(realtimeMessage(statusClock, segment.offset + within));
                pulses_.fetch_add(1, std::memory_order_relaxed);
                ++nextPulse_;
            }
        }
    } else if (!playing) {
        // Free-running. There is no timeline to read pulses off, so they are
        // spaced by the tempo under the playhead — which is what tells an
        // arpeggiator what tempo to be at before anyone presses play.
        const double beatsPerMinute = std::max(tempoMap.tempoAtFrame(blockStart), 1.0);
        const double framesPerPulse = rate * 60.0 / (beatsPerMinute * 24.0);

        if (framesPerPulse >= 1.0) {
            stoppedPhaseFrames_ += static_cast<double>(blockSize);

            for (int guard = 0; guard < maxPulsesPerBlock && stoppedPhaseFrames_ >= framesPerPulse; ++guard) {
                stoppedPhaseFrames_ -= framesPerPulse;

                const auto offset = static_cast<FrameCount>(
                    std::clamp(static_cast<double>(blockSize) - stoppedPhaseFrames_, 0.0,
                               static_cast<double>(blockSize - 1)));

                (void)destination.insert(realtimeMessage(statusClock, offset));
                pulses_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    wasPlaying_ = playing;
    hasHistory_ = true;

    // Where the next block is expected to begin. Anything else is a jump.
    if (hasPlan) {
        const BlockSegment& last = segments[segmentCount - 1];
        expectedFrame_ = last.startFrame + last.length;
    } else {
        expectedFrame_ = blockStart;
    }
}

} // namespace incdaw::engine
