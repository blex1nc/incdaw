#include "engine/audio/AudioClipNode.h"

namespace incdaw::engine {

void AudioClipNode::addClip(PlacedClip clip)
{
    if ((clip.audio == nullptr && clip.stream == nullptr) || clip.length <= 0)
        return;

    clips_.push_back(std::move(clip));
}

void AudioClipNode::prepare(SampleRate, FrameCount maxBlockSize)
{
    fetchScratch_.assign(static_cast<std::size_t>(maxBlockSize > 0 ? maxBlockSize : 1), 0.0f);
}

void AudioClipNode::process(const ProcessContext& context) noexcept
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

        const std::size_t sourceChannels =
            clip.audio != nullptr ? clip.audio->channelCount : clip.stream->channelCount();

        if (sourceChannels == 0)
            continue;

        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            // A mono clip plays on every output channel; a stereo one maps
            // channel to channel. Outputs beyond the audio's channels repeat
            // the last one rather than dropping to silence on one side.
            const std::size_t sourceChannel =
                channel < sourceChannels ? channel : sourceChannels - 1;

            // Where this range's samples come from: direct indexing for a
            // preloaded clip, the streamer's window (via the scratch) for a
            // streamed one. Both paths then run the same gain-and-fade loop.
            const Sample* source        = nullptr;
            FrameCount    sourceIndex0  = 0;   ///< source index of `from`

            const FrameCount rangeStart = (from - clip.start) + clip.sourceOffset;
            const FrameCount rangeLength = to - from;

            if (clip.audio != nullptr) {
                source       = clip.audio->channels[sourceChannel].data();
                sourceIndex0 = rangeStart;

                // Clamp against the decoded data; anything outside is silence
                // and skipping the whole channel here matches the old code.
                if (rangeStart >= clip.audio->frameCount)
                    continue;
            } else {
                if (rangeLength > static_cast<FrameCount>(fetchScratch_.size()))
                    continue;   // prepare() was not called with a block this big

                clip.stream->read(rangeStart, rangeLength, sourceChannel, fetchScratch_.data());
                source       = fetchScratch_.data();
                sourceIndex0 = 0;
            }

            const FrameCount sourceEnd = clip.audio != nullptr
                                             ? clip.audio->frameCount
                                             : sourceIndex0 + rangeLength;

            Sample* out = context.output.channel(channel);

            for (FramePosition frame = from; frame < to; ++frame) {
                const FrameCount inClip      = frame - clip.start;
                const FrameCount sourceIndex = sourceIndex0 + (frame - from);

                if (sourceIndex >= sourceEnd)
                    break;   // past the decoded audio: the rest is silence

                Sample value = source[static_cast<std::size_t>(sourceIndex)] * clip.gain;

                // Linear fades, positioned at the clip's edges.
                if (clip.fadeInFrames > 0 && inClip < clip.fadeInFrames)
                    value *= static_cast<Sample>(inClip)
                           / static_cast<Sample>(clip.fadeInFrames);

                const FrameCount fromEnd = clip.length - inClip;
                if (clip.fadeOutFrames > 0 && fromEnd <= clip.fadeOutFrames)
                    value *= static_cast<Sample>(fromEnd)
                           / static_cast<Sample>(clip.fadeOutFrames);

                out[frame - blockStart] += value;   // clips on one track may overlap
            }
        }
    }
}

} // namespace incdaw::engine
