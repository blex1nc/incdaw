#include "engine/audio/AudioClipNode.h"

namespace incdaw::engine {

void AudioClipNode::addClip(PlacedClip clip)
{
    if (clip.audio == nullptr || clip.length <= 0)
        return;

    clips_.push_back(std::move(clip));
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

        const AudioFileData& audio = *clip.audio;

        for (std::size_t channel = 0; channel < context.output.channelCount(); ++channel) {
            // A mono clip plays on every output channel; a stereo one maps
            // channel to channel. Outputs beyond the audio's channels repeat
            // the last one rather than dropping to silence on one side.
            const std::size_t sourceChannel =
                channel < audio.channelCount ? channel : audio.channelCount - 1;
            const auto& source = audio.channels[sourceChannel];

            Sample* out = context.output.channel(channel);

            for (FramePosition frame = from; frame < to; ++frame) {
                const FrameCount inClip   = frame - clip.start;
                const FrameCount inSource = inClip + clip.sourceOffset;

                if (inSource < 0 || inSource >= audio.frameCount)
                    continue;

                Sample value = source[static_cast<std::size_t>(inSource)]
                             * clip.gain;

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
