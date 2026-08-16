// incdaw-bench — the measurements Phase 18 is built on.
//
// CLAUDE.md §27: profiling before optimising, no optimisation without a
// number. This tool produces the numbers: representative workloads, wall
// time via steady_clock, medians over repeats. Results land in
// docs/PERFORMANCE.md next to the optimisation they justified.

#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/Resampler.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/transport/TempoMap.h"
#include "project/Model.h"
#include "project/OfflineRender.h"
#include "project/ProjectGraphCompiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace incdaw;
using Clock = std::chrono::steady_clock;

namespace {

double millisecondsSince(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

double median(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

/// A deliberately heavy but realistic project: `channels` sampler channels
/// over a shared decoded sample, notes across two bars, an EQ + compressor
/// on every fourth mixer strip, arrangement mode.
project::Project makeHeavyProject(int channels, engine::SampleCache& cache,
                                  const std::filesystem::path& wavPath)
{
    project::Project heavy;

    // One shared one-second asset, written once.
    {
        engine::AudioFileData data;
        data.sampleRate   = 48000.0;
        data.channelCount = 1;
        data.frameCount   = 48000;
        data.channels.assign(1, std::vector<engine::Sample>(48000));
        for (std::size_t frame = 0; frame < 48000; ++frame)
            data.channels[0][frame] = static_cast<engine::Sample>(
                0.25 * std::sin(2.0 * 3.14159265358979 * 220.0 * static_cast<double>(frame)
                                / 48000.0));
        (void)engine::WavFile::write(wavPath, data);
        std::string error;
        (void)cache.load(wavPath, error);
    }

    auto& asset        = heavy.addAudioAsset(wavPath.string());
    asset.absolutePath = wavPath.string();
    const auto assetId = asset.id;

    auto& track = heavy.addTrack(project::TrackType::instrument, "T");

    auto& pattern  = heavy.addPattern("P");
    pattern.length = engine::ticksPerQuarterNote * 8;

    for (int index = 0; index < channels; ++index) {
        auto& channel      = heavy.addChannel("Ch " + std::to_string(index));
        channel.instrument = plugins::builtinSampler();

        project::ChannelSamplerZone zone;
        zone.asset   = assetId;
        zone.rootKey = 60;
        heavy.channels().back().samplerZones.push_back(zone);

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = (index % 8) * engine::ticksPerQuarterNote;
        note.duration = engine::ticksPerQuarterNote * 2;
        note.key      = 48 + (index % 24);
        note.value    = 100;
        pattern.contentFor(heavy.channels().back().id).events.push_back(note);
    }

    // Effects on the master: EQ, compressor, reverb — a typical bus chain.
    project::MixerNode* master = heavy.findMixerNode(heavy.masterMixerNode());
    for (const char* uid : {"incdaw.eq", "incdaw.compressor", "incdaw.reverb"}) {
        project::PluginSlot slot;
        slot.id     = heavy.ids().next();
        slot.plugin = {plugins::Format::builtin, uid};
        master->inserts.push_back(slot);
    }

    auto& clip       = heavy.addClip(project::ClipType::pattern, track.id, pattern.id);
    clip.startTick   = 0;
    clip.lengthTicks = pattern.length;

    return heavy;
}

} // namespace

int main()
{
    const auto scratch = std::filesystem::temp_directory_path() / "incdaw-bench";
    std::filesystem::create_directories(scratch);
    const auto wavPath = scratch / "tone.wav";

    engine::SampleCache    cache;
    const engine::TempoMap map{120.0, 48000.0};

    std::printf("incdaw-bench (Release numbers are the ones that count)\n\n");

    // ── Graph compile time: the cost of every edit ───────────────────────
    for (const int channels : {16, 64}) {
        project::Project heavy = makeHeavyProject(channels, cache, wavPath);

        project::GraphCompileOptions options;
        options.source      = project::PlaybackSource::arrangement;
        options.sampleCache = &cache;

        std::vector<double> times;
        for (int run = 0; run < 20; ++run) {
            const auto start    = Clock::now();
            const auto compiled = project::compileProjectGraph(heavy, map, options);
            times.push_back(millisecondsSince(start));
            if (!compiled)
                return 1;
        }

        std::printf("compile %3d sampler channels + master chain: %8.3f ms/rebuild\n",
                    channels, median(times));
    }

    // ── Render throughput: how much faster than realtime ────────────────
    {
        project::Project heavy = makeHeavyProject(64, cache, wavPath);

        project::RenderOptions options;
        options.tailSeconds = 0.0;
        options.sampleCache = &cache;

        const auto start    = Clock::now();
        const auto rendered = project::renderProject(heavy, map, options);
        const auto elapsed  = millisecondsSince(start);
        if (!rendered)
            return 1;

        const double audioSeconds =
            static_cast<double>(rendered.audio.frameCount) / options.sampleRate;
        std::printf("render 64 channels, %4.1f s of audio:          %8.3f ms  (%.1fx realtime)\n",
                    audioSeconds, elapsed, audioSeconds * 1000.0 / elapsed);
    }

    // ── Per-effect block cost ────────────────────────────────────────────
    {
        constexpr engine::FrameCount blockSize = 512;
        constexpr int                blocks    = 4000;

        engine::AudioBufferPool pool;
        pool.allocate(2, 2, blockSize);

        const auto input  = pool.buffer(0);
        const auto output = pool.buffer(1);

        std::uint32_t noise = 0x1234567u;
        for (std::size_t channel = 0; channel < 2; ++channel)
            for (engine::FrameCount frame = 0; frame < blockSize; ++frame) {
                noise = noise * 1664525u + 1013904223u;
                input.channel(channel)[frame] =
                    static_cast<engine::Sample>(static_cast<double>(noise) / 4294967295.0 - 0.5);
            }

        std::printf("\nper-effect cost, 512-frame stereo blocks:\n");

        for (const auto& info : engine::dsp::builtinEffects()) {
            auto node = engine::dsp::makeBuiltinEffect(info.uid, 48000.0);
            node->prepare(48000.0, blockSize);

            // Non-transparent settings so the real path is measured: 75% of
            // each range, deliberately off-centre — the EQ's mid-range gain
            // is 0 dB, which is a skipped band, and measuring a bypass would
            // be measuring nothing.
            if (engine::ParameterSink* sink = node->parameterSink())
                for (std::size_t index = 0; index < info.parameterCount; ++index)
                    sink->setParameter(info.parameters[index].id,
                                       info.parameters[index].minValue
                                           + (info.parameters[index].maxValue
                                              - info.parameters[index].minValue) * 0.75);

            engine::ProcessContext context;
            context.output     = output;
            context.inputs     = &input;
            context.inputCount = 1;
            context.frameCount = blockSize;
            context.sampleRate = 48000.0;

            const auto start = Clock::now();
            for (int block = 0; block < blocks; ++block) {
                output.clear();
                node->process(context);
            }
            const double elapsed = millisecondsSince(start);

            const double nsPerFrame = elapsed * 1.0e6 / (static_cast<double>(blocks) * blockSize);
            const double blockBudgetPercent =
                elapsed / static_cast<double>(blocks) / (1000.0 * blockSize / 48000.0) * 100.0;

            std::printf("  %-12s %7.1f ns/frame  (%5.2f%% of the block budget)\n",
                        info.displayName, nsPerFrame, blockBudgetPercent);
        }
    }

    // ── Resampler throughput ─────────────────────────────────────────────
    {
        engine::AudioFileData tenSeconds;
        tenSeconds.sampleRate   = 48000.0;
        tenSeconds.channelCount = 2;
        tenSeconds.frameCount   = 480000;
        tenSeconds.channels.assign(2, std::vector<engine::Sample>(480000, 0.1f));

        const auto start     = Clock::now();
        const auto converted = engine::dsp::resample(tenSeconds, 44100.0);
        const auto elapsed   = millisecondsSince(start);

        std::printf("\nresample 10 s stereo 48k -> 44.1k:            %8.3f ms  (%.1fx realtime)\n",
                    elapsed, 10000.0 / elapsed);
        (void)converted;
    }

    return 0;
}
