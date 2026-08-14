#include "engine/audio/WaveformOverview.h"

#include "engine/audio/WavStreamReader.h"

#include <algorithm>

namespace incdaw::engine {
namespace {

void bucketize(const Sample* samples, FrameCount count, FrameCount firstFrame,
               FrameCount framesPerBucket, std::vector<WaveformOverview::Bucket>& out)
{
    for (FrameCount index = 0; index < count; ++index) {
        const auto bucket = static_cast<std::size_t>((firstFrame + index) / framesPerBucket);
        if (bucket >= out.size())
            break;

        WaveformOverview::Bucket& entry = out[bucket];
        const Sample value = samples[static_cast<std::size_t>(index)];

        entry.low  = std::min(entry.low, value);
        entry.high = std::max(entry.high, value);
    }
}

void sizeBuckets(WaveformOverview& out)
{
    const auto buckets = static_cast<std::size_t>(
        (out.frameCount + out.framesPerBucket - 1) / out.framesPerBucket);

    out.channels.assign(out.channelCount, std::vector<WaveformOverview::Bucket>(buckets));
}

} // namespace

WavFile::Result WaveformOverview::build(const std::filesystem::path& path, WaveformOverview& out,
                                        FrameCount framesPerBucket)
{
    WavStreamReader reader;

    WavFile::Result result = reader.open(path);
    if (!result)
        return result;

    out.framesPerBucket = framesPerBucket > 0 ? framesPerBucket : 1;
    out.frameCount      = reader.frameCount();
    out.sampleRate      = reader.sampleRate();
    out.channelCount    = reader.channelCount();
    sizeBuckets(out);

    // One chunk at a time, decoded and folded straight into buckets: the
    // resident cost is the chunk, never the file.
    constexpr FrameCount chunkFrames = 65536;

    std::vector<std::vector<Sample>> chunk(out.channelCount,
                                           std::vector<Sample>(chunkFrames));
    std::vector<Sample*> pointers(out.channelCount);
    for (std::size_t channel = 0; channel < out.channelCount; ++channel)
        pointers[channel] = chunk[channel].data();

    for (FrameCount position = 0; position < out.frameCount; position += chunkFrames) {
        const FrameCount frames = std::min(chunkFrames, out.frameCount - position);

        if (!reader.readAt(position, frames, pointers.data(), out.channelCount)) {
            result.succeeded = false;
            result.error     = "read failed while building overview: " + path.string();
            return result;
        }

        for (std::size_t channel = 0; channel < out.channelCount; ++channel)
            bucketize(pointers[channel], frames, position, out.framesPerBucket,
                      out.channels[channel]);
    }

    return result;
}

void WaveformOverview::build(const AudioFileData& data, WaveformOverview& out,
                             FrameCount framesPerBucket)
{
    out.framesPerBucket = framesPerBucket > 0 ? framesPerBucket : 1;
    out.frameCount      = data.frameCount;
    out.sampleRate      = data.sampleRate;
    out.channelCount    = data.channelCount;
    sizeBuckets(out);

    for (std::size_t channel = 0; channel < data.channelCount; ++channel)
        bucketize(data.channels[channel].data(), data.frameCount, 0, out.framesPerBucket,
                  out.channels[channel]);
}

} // namespace incdaw::engine
