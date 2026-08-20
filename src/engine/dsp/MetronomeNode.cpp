#include "engine/dsp/MetronomeNode.h"

#include <cmath>
#include <numbers>

namespace incdaw::engine::dsp {
namespace {

constexpr double clickSeconds        = 0.030;
constexpr double downbeatFrequency   = 1600.0;
constexpr double offbeatFrequency    = 1000.0;

} // namespace

void MetronomeNode::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    sampleRate_  = sampleRate > 0.0 ? sampleRate : 48000.0;
    clickLength_ = static_cast<FrameCount>(clickSeconds * sampleRate_);

    clickRemaining_ = 0;
    clickPhase_     = 0.0;
    clickCount_     = 0;

    // Sized here, on a non-realtime thread. The worst case is one click per
    // frame, which cannot happen musically, so the block size is a safe bound
    // and the vector never grows during processing.
    clickOffsets_.assign(static_cast<std::size_t>(maxBlockSize > 0 ? maxBlockSize : 1), 0);
}

void MetronomeNode::triggerClick(bool isDownbeat) noexcept
{
    clickRemaining_ = clickLength_;
    clickPhase_     = 0.0;
    clickIncrement_ = (isDownbeat ? downbeatFrequency : offbeatFrequency) / sampleRate_;
}

void MetronomeNode::process(const ProcessContext& context) noexcept
{
    clickCount_ = 0;

    if (tempoMap_ == nullptr || !enabled_.load(std::memory_order_relaxed))
        return;

    const TempoMap& tempoMap = *tempoMap_;

    const FramePosition blockStart = context.playPosition;
    const FramePosition blockEnd   = blockStart + context.frameCount;

    // ── Find the beats that fall inside this block ───────────────────────────
    // A parked playhead would otherwise click on the beat it sits on, once per
    // block, forever (docs/AUDIO_ENGINE.md §5).
    if (context.playing) {
        const Tick          startTick = tempoMap.tickForFrame(blockStart);
        const TimeSignature signature = tempoMap.timeSignatureAtTick(startTick);
        const Tick          beatTicks = signature.ticksPerBeat();

        if (beatTicks > 0) {
            // First beat at or after the block start. Floor division so that a
            // negative position (count-in) still lands on the right beat.
            Tick beat = (startTick >= 0 ? (startTick + beatTicks - 1) / beatTicks
                                        : startTick / beatTicks) * beatTicks;

            while (beat < startTick)
                beat += beatTicks;

            for (int guard = 0; guard < 1024; ++guard) {
                const FramePosition beatFrame = tempoMap.frameForTick(beat);

                if (beatFrame >= blockEnd)
                    break;

                if (beatFrame >= blockStart) {
                    const FrameCount offset = beatFrame - blockStart;

                    const Tick barTicks    = signature.ticksPerBar();
                    const bool isDownbeat  = barTicks > 0 && (beat % barTicks) == 0;

                    triggerClick(isDownbeat);

                    if (clickCount_ < clickOffsets_.size())
                        clickOffsets_[clickCount_++] = offset;

                    // Render this click's tail from its own onset. A second beat
                    // inside the same block simply retriggers, which is what a
                    // metronome does at very fast tempi.
                    Sample* first = context.output.channelCount() > 0 ? context.output.channel(0) : nullptr;
                    if (first != nullptr) {
                        const Sample amplitude = amplitude_.load(std::memory_order_relaxed);

                        for (FrameCount frame = offset; frame < context.frameCount && clickRemaining_ > 0;
                             ++frame, --clickRemaining_) {
                            const double decay = static_cast<double>(clickRemaining_)
                                               / static_cast<double>(clickLength_ > 0 ? clickLength_ : 1);

                            first[frame] += static_cast<Sample>(
                                static_cast<double>(amplitude) * decay
                                * std::sin(2.0 * std::numbers::pi * clickPhase_));

                            clickPhase_ += clickIncrement_;
                            if (clickPhase_ >= 1.0)
                                clickPhase_ -= std::floor(clickPhase_);
                        }
                    }
                }

                beat += beatTicks;
            }
        }
    }

    // ── Continue a click that began in an earlier block ──────────────────────
    if (clickRemaining_ > 0 && clickCount_ == 0 && context.output.channelCount() > 0) {
        Sample*      first     = context.output.channel(0);
        const Sample amplitude = amplitude_.load(std::memory_order_relaxed);

        for (FrameCount frame = 0; frame < context.frameCount && clickRemaining_ > 0;
             ++frame, --clickRemaining_) {
            const double decay = static_cast<double>(clickRemaining_)
                               / static_cast<double>(clickLength_ > 0 ? clickLength_ : 1);

            first[frame] += static_cast<Sample>(
                static_cast<double>(amplitude) * decay
                * std::sin(2.0 * std::numbers::pi * clickPhase_));

            clickPhase_ += clickIncrement_;
            if (clickPhase_ >= 1.0)
                clickPhase_ -= std::floor(clickPhase_);
        }
    }

    // Mirror to the remaining channels.
    for (std::size_t channel = 1; channel < context.output.channelCount(); ++channel) {
        Sample*       destination = context.output.channel(channel);
        const Sample* source      = context.output.channel(0);

        for (FrameCount frame = 0; frame < context.frameCount; ++frame)
            destination[frame] = source[frame];
    }
}

} // namespace incdaw::engine::dsp
