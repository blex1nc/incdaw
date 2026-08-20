#include "engine/audio/AuditionPlayer.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine {

void AuditionPlayer::play(std::shared_ptr<const AudioFileData> audio, SampleRate outputRate,
                          Sample gain, std::uint64_t blockCounter)
{
    if (audio == nullptr || audio->frameCount <= 0 || audio->channelCount == 0
        || audio->channels.size() < audio->channelCount) {
        stop();
        return;
    }

    // Silenced FIRST, then re-armed last. The audio thread checks `playing_`
    // before it reads anything else, so while the swap is in flight it renders
    // nothing at all — and when it next sees `playing_`, the new pointer and
    // the new generation are already visible to it.
    playing_.store(false, std::memory_order_release);

    if (current_ != nullptr && current_ != audio)
        retired_.push_back(Retired{std::move(current_), blockCounter});

    current_ = std::move(audio);

    const double ratio = outputRate > 0.0 && current_->sampleRate > 0.0
                             ? current_->sampleRate / outputRate
                             : 1.0;

    rateRatio_.store(ratio, std::memory_order_relaxed);
    gain_.store(gain, std::memory_order_relaxed);
    source_.store(current_.get(), std::memory_order_release);
    generation_.fetch_add(1, std::memory_order_release);
    playing_.store(true, std::memory_order_release);
}

void AuditionPlayer::render(const AudioBufferView& output) noexcept
{
    if (!playing_.load(std::memory_order_acquire) || output.isEmpty())
        return;

    const AudioFileData* source = source_.load(std::memory_order_acquire);

    if (source == nullptr || source->frameCount <= 0 || source->channelCount == 0)
        return;

    // A new file resets the read position. Doing it here rather than in play()
    // keeps every write to the playhead on the audio thread.
    if (const std::uint32_t generation = generation_.load(std::memory_order_acquire);
        generation != seenGeneration_) {
        seenGeneration_ = generation;
        position_       = 0.0;
    }

    const double ratio     = rateRatio_.load(std::memory_order_relaxed);
    const auto   gain      = static_cast<double>(gain_.load(std::memory_order_relaxed));
    const auto   lastFrame = static_cast<double>(source->frameCount - 1);
    const auto   frames    = static_cast<std::size_t>(source->frameCount);

    for (FrameCount frame = 0; frame < output.frameCount(); ++frame) {
        if (position_ > lastFrame) {
            // Finished mid-block: the rest of the block is the project's alone.
            playing_.store(false, std::memory_order_release);
            return;
        }

        const auto   index    = static_cast<std::size_t>(position_);
        const double fraction = position_ - static_cast<double>(index);
        const std::size_t next = index + 1 < frames ? index + 1 : index;

        for (std::size_t channel = 0; channel < output.channelCount(); ++channel) {
            // A mono file feeds every output channel; a file with fewer
            // channels than the device repeats its last one, rather than
            // leaving a silent speaker.
            const std::size_t sourceChannel =
                channel < source->channelCount ? channel : source->channelCount - 1;

            const std::vector<Sample>& samples = source->channels[sourceChannel];

            if (samples.size() < frames)
                continue;

            const double value = static_cast<double>(samples[index]) * (1.0 - fraction)
                                 + static_cast<double>(samples[next]) * fraction;

            output.channel(channel)[frame] += static_cast<Sample>(value * gain);
        }

        position_ += ratio;
    }

    if (position_ > lastFrame)
        playing_.store(false, std::memory_order_release);
}

void AuditionPlayer::collect(std::uint64_t blockCounter, bool stopped)
{
    const auto finished = [blockCounter, stopped](const Retired& entry) {
        return stopped || blockCounter >= entry.retiredAtBlock + retirementGraceBlocks;
    };

    retired_.erase(std::remove_if(retired_.begin(), retired_.end(), finished), retired_.end());
}

} // namespace incdaw::engine
