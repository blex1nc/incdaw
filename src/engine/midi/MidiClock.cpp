#include "engine/midi/MidiClock.h"

#include "engine/transport/TempoMap.h"

#include <algorithm>
#include <cmath>

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

// ── Receiving ────────────────────────────────────────────────────────────────

void MidiClockReceiver::reset() noexcept
{
    elapsedFrames_  = 0;
    lastPulseFrame_ = 0;
    hasPulse_       = false;
    intervalFrames_ = 0.0;
    acceptedRun_    = 0;
    rejectedRun_    = 0;
    seedSum_        = 0.0;
    seedCount_      = 0;

    estimatedTempo_.store(0.0, std::memory_order_relaxed);
    locked_.store(false, std::memory_order_relaxed);
    externalTick_.store(0, std::memory_order_relaxed);
    pulses_.store(0, std::memory_order_relaxed);
    rejected_.store(0, std::memory_order_relaxed);
}

void MidiClockReceiver::acceptInterval(double intervalFrames, SampleRate sampleRate) noexcept
{
    if (seedCount_ < seedIntervals) {
        // Still finding the rate. A plain mean, because there is nothing yet
        // to judge an outlier against.
        seedSum_ += intervalFrames;
        ++seedCount_;
        intervalFrames_ = seedSum_ / static_cast<double>(seedCount_);
    } else {
        intervalFrames_ += (intervalFrames - intervalFrames_) * smoothing;
    }

    if (intervalFrames_ <= 0.0 || sampleRate <= 0.0)
        return;

    // 24 pulses to the quarter: one pulse every (60 / bpm / 24) seconds.
    const double beatsPerMinute = sampleRate * 60.0 / (intervalFrames_ * 24.0);
    estimatedTempo_.store(beatsPerMinute, std::memory_order_relaxed);

    if (acceptedRun_ >= pulsesToLock)
        locked_.store(true, std::memory_order_relaxed);
}

void MidiClockReceiver::process(const MidiBuffer& incoming, Transport& transport,
                                FrameCount blockSize, SampleRate sampleRate) noexcept
{
    if (role_.load(std::memory_order_relaxed) != MidiClockRole::receive || blockSize <= 0)
        return;

    for (const MidiMessage& message : incoming) {
        const FrameCount at = elapsedFrames_ + message.frameOffset;

        switch (message.status) {
        case 0xFA: {
            // Start: from the beginning. The spec has motion begin on the
            // following pulse; beginning here instead costs at most one pulse
            // and keeps the transport's own seek-then-play ordering intact.
            externalTick_.store(0, std::memory_order_relaxed);
            transport.seek(0);
            transport.play();

            hasPulse_ = false;   // the phase belongs to the run that just ended
            break;
        }

        case 0xFB:
            transport.play();
            hasPulse_ = false;
            break;

        case 0xFC:
            transport.stop();
            locked_.store(false, std::memory_order_relaxed);
            hasPulse_ = false;
            break;

        case 0xF2: {
            // Song position pointer: 14 bits of MIDI beats, which are
            // sixteenth notes, little end first.
            const int  beats = message.data1 | (static_cast<int>(message.data2) << 7);
            const Tick tick  = static_cast<Tick>(beats) * ticksPerSongPositionBeat;

            externalTick_.store(tick, std::memory_order_relaxed);
            transport.seekToTick(tick);
            break;
        }

        case 0xF8: {
            pulses_.fetch_add(1, std::memory_order_relaxed);
            externalTick_.fetch_add(ticksPerPulse, std::memory_order_relaxed);

            if (hasPulse_) {
                const auto interval = static_cast<double>(at - lastPulseFrame_);

                if (interval > 0.0) {
                    // Nothing is rejected while the seed is still being
                    // gathered: the estimate has no authority to reject with.
                    const bool plausible =
                        seedCount_ < seedIntervals
                        || std::abs(interval - intervalFrames_) <= intervalFrames_ * toleranceRatio;

                    if (plausible) {
                        ++acceptedRun_;
                        rejectedRun_ = 0;
                        acceptInterval(interval, sampleRate);
                    } else {
                        rejected_.fetch_add(1, std::memory_order_relaxed);
                        acceptedRun_ = 0;

                        // A run of them is not slop: the master changed tempo,
                        // and an outlier filter that never gives in would sit
                        // at the old one forever.
                        if (++rejectedRun_ >= rejectionsBeforeReseed) {
                            intervalFrames_ = 0.0;
                            rejectedRun_    = 0;
                            seedSum_        = 0.0;
                            seedCount_      = 0;
                            locked_.store(false, std::memory_order_relaxed);
                            acceptInterval(interval, sampleRate);
                        }
                    }
                }
            }

            lastPulseFrame_ = at;
            hasPulse_       = true;
            break;
        }

        default:
            break;   // channel traffic, and the realtime messages we ignore
        }
    }

    elapsedFrames_ += blockSize;

    // The master has gone quiet. The transport is deliberately left running —
    // only an explicit stop stops it, and a cable knocked out mid-take should
    // not silence the session — but nothing is being measured any more, so the
    // estimate stops claiming to be locked.
    if (hasPulse_ && sampleRate > 0.0
        && static_cast<double>(elapsedFrames_ - lastPulseFrame_) > sampleRate * dropoutSeconds) {
        locked_.store(false, std::memory_order_relaxed);
        hasPulse_ = false;
    }
}

} // namespace incdaw::engine
