#pragma once

#include "engine/audio/WavFile.h"

#include <filesystem>
#include <vector>

namespace incdaw::engine {

/// Min/max buckets of an audio file — what a waveform view draws.
///
/// Built through WavStreamReader in chunks, so an hour-long file yields its
/// overview without ever being resident; or from in-memory data after an
/// edit, so the view refreshes without a round trip through the disk. One
/// base resolution: the UI aggregates buckets when zoomed out and switches
/// to samples only when zoomed far past bucket resolution.
struct WaveformOverview {
    struct Bucket {
        Sample low  = 0.0f;
        Sample high = 0.0f;
    };

    FrameCount  framesPerBucket = 0;
    FrameCount  frameCount      = 0;
    SampleRate  sampleRate      = 0.0;
    std::size_t channelCount    = 0;

    /// channelCount vectors of ceil(frameCount / framesPerBucket) buckets.
    std::vector<std::vector<Bucket>> channels;

    [[nodiscard]] std::size_t bucketCount() const noexcept
    {
        return channels.empty() ? 0 : channels.front().size();
    }

    /// Builds from a file without loading it whole. 256 frames per bucket is
    /// ~5 ms at 48 kHz — finer than any full-clip zoom needs.
    [[nodiscard]] static WavFile::Result build(const std::filesystem::path& path,
                                               WaveformOverview& out,
                                               FrameCount framesPerBucket = 256);

    /// Builds from audio already in memory (post-edit refresh).
    static void build(const AudioFileData& data, WaveformOverview& out,
                      FrameCount framesPerBucket = 256);
};

} // namespace incdaw::engine
