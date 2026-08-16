#include "engine/audio/OnsetDetection.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::audio {

std::vector<FrameCount> detectOnsets(const AudioFileData& audio, double sensitivity)
{
    std::vector<FrameCount> onsets;

    if (audio.frameCount == 0 || audio.channelCount == 0 || audio.sampleRate <= 0.0)
        return onsets;

    const auto frames = static_cast<std::size_t>(audio.frameCount);
    const auto block =
        std::max<std::size_t>(1, static_cast<std::size_t>(audio.sampleRate * 0.005));
    const auto minimumGap =
        static_cast<std::size_t>(audio.sampleRate * 0.030);

    // The leap a block must make over its predecessor. Sensitivity 1 demands
    // 9x the energy; higher sensitivity lowers the bar.
    const double leap = 9.0 / std::max(0.1, sensitivity);

    double      previousEnergy = 1.0e-9;
    std::size_t lastOnset      = 0;
    bool        any            = false;

    for (std::size_t start = 0; start + block <= frames; start += block) {
        double energy = 1.0e-9;
        double peak   = 0.0;

        for (std::size_t frame = start; frame < start + block; ++frame) {
            double mixed = 0.0;
            for (const auto& channel : audio.channels)
                mixed += static_cast<double>(channel[frame]);

            energy += mixed * mixed;
            peak = std::max(peak, std::fabs(mixed));
        }

        const bool leaps = energy > previousEnergy * leap && energy > 1.0e-6;
        previousEnergy   = energy;

        if (!leaps)
            continue;

        if (any && start - lastOnset < minimumGap)
            continue;

        // Refine within the block: the attack is the first frame carrying a
        // quarter of the block's peak.
        std::size_t refined = start;
        for (std::size_t frame = start; frame < start + block; ++frame) {
            double mixed = 0.0;
            for (const auto& channel : audio.channels)
                mixed += static_cast<double>(channel[frame]);

            if (std::fabs(mixed) >= peak * 0.25) {
                refined = frame;
                break;
            }
        }

        onsets.push_back(static_cast<FrameCount>(refined));
        lastOnset = start;
        any       = true;
    }

    return onsets;
}

} // namespace incdaw::engine::audio
