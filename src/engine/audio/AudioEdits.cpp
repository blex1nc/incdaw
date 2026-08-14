#include "engine/audio/AudioEdits.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::edits {

Region clampedRegion(const AudioFileData& data, Region region) noexcept
{
    Region clamped;
    clamped.from = std::max<FrameCount>(0, std::min(region.from, data.frameCount));
    clamped.to   = std::max(clamped.from, std::min(region.to, data.frameCount));
    return clamped;
}

void applyGain(AudioFileData& data, Region region, Sample factor) noexcept
{
    const Region clamped = clampedRegion(data, region);

    for (auto& channel : data.channels)
        for (FrameCount frame = clamped.from; frame < clamped.to; ++frame)
            channel[static_cast<std::size_t>(frame)] *= factor;
}

Sample peakIn(const AudioFileData& data, Region region) noexcept
{
    const Region clamped = clampedRegion(data, region);

    Sample peak = 0.0f;

    for (const auto& channel : data.channels)
        for (FrameCount frame = clamped.from; frame < clamped.to; ++frame)
            peak = std::max(peak, std::abs(channel[static_cast<std::size_t>(frame)]));

    return peak;
}

bool normalize(AudioFileData& data, Region region, Sample targetPeak) noexcept
{
    const Sample peak = peakIn(data, region);
    if (peak <= 0.0f)
        return false;

    applyGain(data, region, targetPeak / peak);
    return true;
}

void reverse(AudioFileData& data, Region region) noexcept
{
    const Region clamped = clampedRegion(data, region);

    for (auto& channel : data.channels)
        std::reverse(channel.begin() + static_cast<std::ptrdiff_t>(clamped.from),
                     channel.begin() + static_cast<std::ptrdiff_t>(clamped.to));
}

void silence(AudioFileData& data, Region region) noexcept
{
    const Region clamped = clampedRegion(data, region);

    for (auto& channel : data.channels)
        std::fill(channel.begin() + static_cast<std::ptrdiff_t>(clamped.from),
                  channel.begin() + static_cast<std::ptrdiff_t>(clamped.to), 0.0f);
}

namespace {

/// Both fades share one ramp; only the direction differs. The denominator is
/// length, not length - 1, so a fade-in's first frame is exactly zero and a
/// fade-out's last frame is one step above it — matching AudioClipNode's
/// clip-edge fades, which the editor's rendered fades must sound like.
void applyRamp(AudioFileData& data, Region region, bool rising) noexcept
{
    const Region clamped = clampedRegion(data, region);
    const FrameCount length = clamped.length();

    if (length <= 0)
        return;

    for (auto& channel : data.channels) {
        for (FrameCount frame = clamped.from; frame < clamped.to; ++frame) {
            const FrameCount position = frame - clamped.from;
            const Sample ramp = rising
                ? static_cast<Sample>(position) / static_cast<Sample>(length)
                : static_cast<Sample>(length - position) / static_cast<Sample>(length);

            channel[static_cast<std::size_t>(frame)] *= ramp;
        }
    }
}

} // namespace

void fadeIn(AudioFileData& data, Region region) noexcept
{
    applyRamp(data, region, true);
}

void fadeOut(AudioFileData& data, Region region) noexcept
{
    applyRamp(data, region, false);
}

void trimTo(AudioFileData& data, Region region)
{
    const Region clamped = clampedRegion(data, region);

    for (auto& channel : data.channels) {
        channel.erase(channel.begin() + static_cast<std::ptrdiff_t>(clamped.to), channel.end());
        channel.erase(channel.begin(), channel.begin() + static_cast<std::ptrdiff_t>(clamped.from));
    }

    data.frameCount = clamped.length();
}

} // namespace incdaw::engine::edits
