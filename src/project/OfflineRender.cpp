#include "project/OfflineRender.h"

#include "engine/audio/AiffFile.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/Resampler.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace incdaw::project {

namespace {

/// Deterministic TPDF dither: two independent uniform randoms per sample,
/// scaled to one LSB. xorshift because it is tiny, seedable and more than
/// random enough for dither.
class DitherSource {
public:
    explicit DitherSource(std::uint64_t seed) : state_(seed | 1) {}

    [[nodiscard]] double nextTpdf() noexcept
    {
        return uniform() + uniform() - 1.0;   // triangular in [-1, 1]
    }

private:
    [[nodiscard]] double uniform() noexcept
    {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 7;
        state_ ^= state_ << 17;
        return static_cast<double>(state_ >> 11)
             / static_cast<double>(std::uint64_t{1} << 53);
    }

    std::uint64_t state_;
};

} // namespace

engine::FrameCount arrangementEndFrames(const Project& project, const engine::TempoMap& tempoMap)
{
    engine::FrameCount end = 0;

    for (const Clip& clip : project.clips()) {
        if (clip.type == ClipType::audio) {
            end = std::max(end, static_cast<engine::FrameCount>(clip.start) + clip.length);
        } else {
            const Tick endTick = clipStartTicks(clip, tempoMap) + clipLengthTicks(clip, tempoMap);
            end = std::max(end, static_cast<engine::FrameCount>(tempoMap.frameForTick(endTick)));
        }
    }

    return end;
}

RenderResult renderProject(const Project& project, const engine::TempoMap& tempoMap,
                           const RenderOptions& options)
{
    RenderResult result;

    if (options.sampleRate <= 0.0 || options.blockSize <= 0) {
        result.error = "render needs a positive sample rate and block size";
        return result;
    }

    // Stems and individual tracks are solo states on a copy: solo is already
    // the engine's answer to "only this one", so a stem cannot drift from
    // what soloing in the mixer sounds like.
    const Project* toRender = &project;
    Project        copy;

    if (options.stemMixerNode.isValid() || options.soloChannel.isValid()) {
        copy = project;

        if (options.stemMixerNode.isValid()) {
            MixerNode* node = copy.findMixerNode(options.stemMixerNode);
            if (node == nullptr) {
                result.error = "stem target does not exist";
                return result;
            }
            node->soloed = true;
        }

        if (options.soloChannel.isValid()) {
            Channel* channel = copy.findChannel(options.soloChannel);
            if (channel == nullptr) {
                result.error = "solo channel does not exist";
                return result;
            }
            channel->soloed = true;
        }

        toRender = &copy;
    }

    GraphCompileOptions compile;
    compile.sampleRate   = options.sampleRate;
    compile.maxBlockSize = options.blockSize;
    compile.source       = PlaybackSource::arrangement;
    compile.randomSeed   = options.randomSeed;
    compile.sampleCache  = options.sampleCache;
    compile.parameters   = options.parameters;
    // Deliberately no diskStreamer: offline preloads everything (determinism).

    const auto compiled = compileProjectGraph(*toRender, tempoMap, compile);
    if (!compiled) {
        result.error = compiled.error.empty() ? "project did not compile" : compiled.error;
        return result;
    }

    result.warnings = compiled.warnings;

    // ── Length ───────────────────────────────────────────────────────────
    const engine::FrameCount arrangementEnd = arrangementEndFrames(*toRender, tempoMap);

    engine::FramePosition start  = 0;
    engine::FrameCount    length = 0;

    if (options.regionLength > 0) {
        start  = options.regionStart;
        length = options.regionLength;
    } else {
        length = arrangementEnd;
    }

    result.arrangementFrames = length;

    const auto tailFrames =
        static_cast<engine::FrameCount>(options.tailSeconds * options.sampleRate);
    const engine::FrameCount totalFrames = length + tailFrames;

    if (totalFrames <= 0) {
        result.error = "nothing to render: the arrangement is empty and there is no tail";
        return result;
    }

    // ── The render loop: exactly the audio callback's shape ─────────────
    constexpr std::size_t channels = 2;

    engine::AudioBufferPool pool;
    pool.allocate(1, channels, options.blockSize);

    result.audio.sampleRate   = options.sampleRate;
    result.audio.channelCount = channels;
    result.audio.frameCount   = totalFrames;
    result.audio.channels.assign(channels,
                                 std::vector<engine::Sample>(
                                     static_cast<std::size_t>(totalFrames)));

    for (engine::FrameCount rendered = 0; rendered < totalFrames;) {
        const engine::FrameCount count =
            std::min<engine::FrameCount>(options.blockSize, totalFrames - rendered);

        const auto view = pool.buffer(0);
        compiled.graph->process(view, count, start + rendered);

        for (std::size_t channel = 0; channel < channels; ++channel)
            std::copy_n(view.channel(channel), count,
                        result.audio.channels[channel].begin()
                            + static_cast<std::ptrdiff_t>(rendered));

        rendered += count;
    }

    // ── Normalize ────────────────────────────────────────────────────────
    if (options.normalize) {
        engine::Sample peak = 0.0f;
        for (const auto& channel : result.audio.channels)
            for (const engine::Sample sample : channel)
                peak = std::max(peak, std::abs(sample));

        if (peak > 0.0f) {
            const float scale = 1.0f / peak;
            for (auto& channel : result.audio.channels)
                for (engine::Sample& sample : channel)
                    sample *= scale;
        }
    }

    // ── Sample-rate conversion ───────────────────────────────────────────
    if (options.targetSampleRate > 0.0 && options.targetSampleRate != options.sampleRate)
        result.audio = engine::dsp::resample(result.audio, options.targetSampleRate);

    // ── Dither, before quantisation ─────────────────────────────────────
    if (options.bitDepth == RenderOptions::BitDepth::pcm16 && options.dither) {
        DitherSource dither(options.randomSeed + 0x9E3779B97F4A7C15ull);
        const double lsb = 1.0 / 32767.0;

        for (auto& channel : result.audio.channels)
            for (engine::Sample& sample : channel)
                sample = static_cast<engine::Sample>(static_cast<double>(sample)
                                                     + dither.nextTpdf() * lsb);
    }

    result.succeeded = true;
    return result;
}

RenderResult renderProjectToFile(const Project& project, const engine::TempoMap& tempoMap,
                                 const RenderOptions& options,
                                 const std::filesystem::path& path)
{
    RenderResult result = renderProject(project, tempoMap, options);
    if (!result)
        return result;

    std::string extension = path.extension().string();
    for (char& character : extension)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    if (extension == ".aiff" || extension == ".aif") {
        const auto format = options.bitDepth == RenderOptions::BitDepth::pcm16
                                ? engine::AiffFile::Format::pcm16
                                : engine::AiffFile::Format::pcm24;

        if (const auto written = engine::AiffFile::write(path, result.audio, format); !written) {
            result.succeeded = false;
            result.error     = written.error;
        }
        return result;
    }

    const auto format = options.bitDepth == RenderOptions::BitDepth::pcm16
                            ? engine::WavFile::Format::pcm16
                        : options.bitDepth == RenderOptions::BitDepth::pcm24
                            ? engine::WavFile::Format::pcm24
                            : engine::WavFile::Format::float32;

    if (const auto written = engine::WavFile::write(path, result.audio, format); !written) {
        result.succeeded = false;
        result.error     = written.error;
    }

    return result;
}

} // namespace incdaw::project
