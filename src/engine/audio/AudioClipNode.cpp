#include "engine/audio/AudioClipNode.h"

#include <algorithm>
#include <utility>

namespace incdaw::engine {

void AudioClipNode::addClip(PlacedClip clip)
{
    if ((clip.audio == nullptr && clip.stream == nullptr) || clip.length <= 0)
        return;

    clips_.push_back(std::move(clip));
}

void AudioClipNode::setPerformance(PerformanceScheduler* scheduler, std::size_t track,
                                   TempoMap tempoMap)
{
    performance_      = scheduler;
    performanceTrack_ = track;
    performanceTempo_ = std::move(tempoMap);
}

void AudioClipNode::prepare(SampleRate, FrameCount maxBlockSize)
{
    fetchScratch_.assign(static_cast<std::size_t>(maxBlockSize > 0 ? maxBlockSize : 1), 0.0f);
}

void AudioClipNode::renderRange(const PlacedClip& clip, const ProcessContext& context,
                                FramePosition blockStart, FramePosition from, FramePosition to,
                                FrameCount sourceStart) noexcept
{
    if (from >= to)
        return;

    const std::size_t sourceChannels =
        clip.audio != nullptr ? clip.audio->channelCount : clip.stream->channelCount();

    if (sourceChannels == 0)
        return;

    for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
        // A mono clip plays on every output channel; a stereo one maps
        // channel to channel. Outputs beyond the audio's channels repeat
        // the last one rather than dropping to silence on one side.
        const std::size_t sourceChannel =
            channel < sourceChannels ? channel : sourceChannels - 1;

        // Pan folds into the clip's gain once per channel, not per sample:
        // the inner loop stays the single multiply it was.
        const Sample gain = clip.gain
                          * (channel == 0   ? clip.panLeft
                             : channel == 1 ? clip.panRight
                                            : Sample{1});

        // Where this range's samples come from: direct indexing for a
        // preloaded clip, the streamer's window (via the scratch) for a
        // streamed one. Both paths then run the same gain-and-fade loop.
        const Sample* source       = nullptr;
        FrameCount    sourceIndex0 = 0;   ///< source index of `from`

        const FrameCount rangeLength = to - from;

        if (clip.audio != nullptr) {
            source       = clip.audio->channels[sourceChannel].data();
            sourceIndex0 = sourceStart;

            // Clamp against the decoded data; anything outside is silence
            // and skipping the whole channel here matches the old code.
            if (sourceStart >= clip.audio->frameCount)
                continue;
        } else {
            if (rangeLength > static_cast<FrameCount>(fetchScratch_.size()))
                continue;   // prepare() was not called with a block this big

            clip.stream->read(sourceStart, rangeLength, sourceChannel, fetchScratch_.data());
            source       = fetchScratch_.data();
            sourceIndex0 = 0;
        }

        const FrameCount sourceEnd = clip.audio != nullptr
                                         ? clip.audio->frameCount
                                         : sourceIndex0 + rangeLength;

        Sample* out = context.output.channel(channel);

        for (FramePosition frame = from; frame < to; ++frame) {
            const FrameCount stepped     = frame - from;
            const FrameCount sourceIndex = sourceIndex0 + stepped;

            if (sourceIndex >= sourceEnd)
                break;   // past the decoded audio: the rest is silence

            Sample value = source[static_cast<std::size_t>(sourceIndex)] * gain;

            // The fades sit at the clip's own edges, measured from where this
            // range's material sits inside it rather than from the range.
            const FrameCount inClip = (sourceStart - clip.sourceOffset) + stepped;

            if (clip.fadeInFrames > 0 && inClip < clip.fadeInFrames)
                value *= static_cast<Sample>(inClip)
                       / static_cast<Sample>(clip.fadeInFrames);

            if (clip.fadeOutFrames > 0 && clip.length > inClip) {
                const FrameCount fromEnd = clip.length - inClip;
                if (fromEnd <= clip.fadeOutFrames)
                    value *= static_cast<Sample>(fromEnd)
                           / static_cast<Sample>(clip.fadeOutFrames);
            }

            out[frame - blockStart] += value;   // clips on one track may overlap
        }
    }
}

void AudioClipNode::processArrangement(const ProcessContext& context) noexcept
{
    const FramePosition blockStart = context.playPosition;
    const FramePosition blockEnd   = blockStart + context.frameCount;

    for (const PlacedClip& clip : clips_) {
        if (clip.muted)
            continue;

        // The window where this clip and this block overlap, in timeline frames.
        const FramePosition from = clip.start > blockStart ? clip.start : blockStart;
        const FramePosition to   = clip.start + clip.length < blockEnd
                                       ? clip.start + clip.length : blockEnd;
        if (from >= to)
            continue;

        renderRange(clip, context, blockStart, from, to,
                    static_cast<FrameCount>(from - clip.start) + clip.sourceOffset);
    }
}

void AudioClipNode::processPerformance(const ProcessContext& context) noexcept
{
    const FramePosition blockStart = context.playPosition;
    const FramePosition blockEnd   = blockStart + context.frameCount;

    // The block is split wherever the table changes, so a clip triggered on a
    // bar line starts on that frame rather than at the top of the block that
    // happens to contain it. Advancing is monotonic, so several nodes walking
    // the same boundaries cost one pass between them.
    FramePosition from = blockStart;

    while (from < blockEnd) {
        performance_->advanceTo(from, performanceTempo_);

        const FramePosition to =
            std::min(blockEnd, performance_->nextBoundaryIn(
                                   from, static_cast<FrameCount>(blockEnd - from)));

        const auto voice = performance_->voiceAt(performanceTrack_, from);

        if (voice.sounding && voice.clip < clips_.size()) {
            const PlacedClip& clip = clips_[voice.clip];

            if (!clip.muted)
                renderRange(clip, context, blockStart, from, to,
                            clip.sourceOffset + voice.sourceFrame);
        }

        // `nextBoundaryIn` never answers with `from`, so this terminates.
        from = to > from ? to : blockEnd;
    }
}

void AudioClipNode::process(const ProcessContext& context) noexcept
{
    // A parked playhead sitting inside a clip would otherwise play the same
    // few hundred frames on every block, forever. Clips have no tail to let
    // ring, so nothing is lost by producing silence until time moves again.
    if (!context.playing)
        return;

    if (performance_ != nullptr)
        processPerformance(context);
    else
        processArrangement(context);
}

} // namespace incdaw::engine
