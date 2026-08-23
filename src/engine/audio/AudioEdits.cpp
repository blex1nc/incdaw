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

void shiftMarkers(AudioFileData& data, FramePosition at, FrameCount delta)
{
    std::vector<AudioMarker> kept;
    kept.reserve(data.markers.size());

    for (AudioMarker marker : data.markers) {
        if (marker.start >= at)
            marker.start += delta;

        if (marker.start < 0)
            continue;

        kept.push_back(marker);
    }

    data.markers = std::move(kept);
}

void removeMarkersIn(AudioFileData& data, Region region)
{
    const FrameCount removed = region.length();
    if (removed <= 0)
        return;

    std::vector<AudioMarker> kept;
    kept.reserve(data.markers.size());

    for (AudioMarker marker : data.markers) {
        const FramePosition start = marker.start;
        const FramePosition end   = marker.end();

        // A point inside the removed span goes with the sound it named.
        if (!marker.isRegion()) {
            if (start >= region.from && start < region.to)
                continue;

            if (start >= region.to)
                marker.start = start - removed;

            kept.push_back(marker);
            continue;
        }

        // A region marker keeps whatever survived. Losing all of it is the
        // only case where it disappears — a span that lost its middle is
        // still a span, and silently deleting it would take the user's
        // annotation along with the audio they meant to remove.
        const FramePosition overlapFrom = std::max<FramePosition>(start, region.from);
        const FramePosition overlapTo   = std::min<FramePosition>(end, region.to);
        const FrameCount    overlap     = std::max<FrameCount>(overlapTo - overlapFrom, 0);

        if (overlap >= marker.length)
            continue;

        if (start >= region.to)
            marker.start = start - removed;
        else if (start > region.from)
            marker.start = region.from;

        marker.length -= overlap;
        kept.push_back(marker);
    }

    data.markers = std::move(kept);
}

void trimTo(AudioFileData& data, Region region)
{
    const Region     clamped  = clampedRegion(data, region);
    const FrameCount previous = data.frameCount;

    for (auto& channel : data.channels) {
        channel.erase(channel.begin() + static_cast<std::ptrdiff_t>(clamped.to), channel.end());
        channel.erase(channel.begin(), channel.begin() + static_cast<std::ptrdiff_t>(clamped.from));
    }

    data.frameCount = clamped.length();

    // The tail first, then the head. Removing the head rebases everything
    // after it, so the tail has to be dealt with while its coordinates still
    // mean what they said.
    removeMarkersIn(data, {clamped.to, previous});
    removeMarkersIn(data, {0, clamped.from});
}

AudioFileData extractRegion(const AudioFileData& data, Region region)
{
    const Region clamped = clampedRegion(data, region);

    AudioFileData piece;
    piece.sampleRate   = data.sampleRate;
    piece.channelCount = data.channelCount;
    piece.frameCount   = clamped.length();

    for (const auto& channel : data.channels)
        piece.channels.emplace_back(
            channel.begin() + static_cast<std::ptrdiff_t>(clamped.from),
            channel.begin() + static_cast<std::ptrdiff_t>(clamped.to));

    return piece;
}

void deleteRegion(AudioFileData& data, Region region)
{
    const Region clamped = clampedRegion(data, region);

    for (auto& channel : data.channels)
        channel.erase(channel.begin() + static_cast<std::ptrdiff_t>(clamped.from),
                      channel.begin() + static_cast<std::ptrdiff_t>(clamped.to));

    data.frameCount -= clamped.length();

    removeMarkersIn(data, clamped);
}

bool insertAudio(AudioFileData& data, FramePosition at, const AudioFileData& piece)
{
    if (piece.channelCount != data.channelCount || piece.sampleRate != data.sampleRate)
        return false;

    const auto clampedAt = static_cast<std::size_t>(
        std::min<FramePosition>(at, static_cast<FramePosition>(data.frameCount)));

    for (std::size_t index = 0; index < data.channels.size(); ++index)
        data.channels[index].insert(
            data.channels[index].begin() + static_cast<std::ptrdiff_t>(clampedAt),
            piece.channels[index].begin(), piece.channels[index].end());

    data.frameCount += piece.frameCount;

    // Inserted audio pushes everything at or after the insertion point later.
    // A marker exactly ON the point moves with the sound that follows it,
    // which is what "insert here" means to the person doing it.
    shiftMarkers(data, static_cast<FramePosition>(clampedAt), piece.frameCount);

    return true;
}

} // namespace incdaw::engine::edits
