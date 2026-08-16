#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/AudioEditCommands.h"
#include "app/commands/ClipCommands.h"
#include "engine/audio/WavFile.h"
#include "engine/core/AudioBufferPool.h"
#include "engine/dsp/TimeStretch.h"
#include "engine/transport/TempoMap.h"
#include "project/ProjectGraphCompiler.h"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

using namespace incdaw;
using namespace incdaw::engine;
using incdaw::engine::dsp::StretchOptions;
using incdaw::engine::dsp::timeStretch;

namespace {

constexpr double sampleRate = 48000.0;

AudioFileData sineData(double frequency, double seconds, double amplitude = 0.5)
{
    AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = 2;
    data.frameCount   = static_cast<FrameCount>(sampleRate * seconds);
    data.channels.assign(2, std::vector<Sample>(static_cast<std::size_t>(data.frameCount)));

    for (std::size_t frame = 0; frame < data.channels[0].size(); ++frame) {
        const double value =
            amplitude
            * std::sin(2.0 * 3.14159265358979 * frequency * static_cast<double>(frame) / sampleRate);
        data.channels[0][frame] = static_cast<Sample>(value);
        data.channels[1][frame] = static_cast<Sample>(value);
    }

    return data;
}

/// Estimated frequency from zero crossings over the middle half of the data —
/// edges excluded, where windowing has its say.
double dominantFrequency(const AudioFileData& data)
{
    const std::vector<Sample>& samples = data.channels[0];
    const std::size_t          from    = samples.size() / 4;
    const std::size_t          to      = samples.size() * 3 / 4;

    std::size_t crossings = 0;
    for (std::size_t frame = from + 1; frame < to; ++frame)
        if ((samples[frame - 1] < 0.0f) != (samples[frame] < 0.0f))
            ++crossings;

    const double seconds = static_cast<double>(to - from) / data.sampleRate;
    return static_cast<double>(crossings) / (2.0 * seconds);
}

double rmsOfMiddle(const AudioFileData& data)
{
    const std::vector<Sample>& samples = data.channels[0];
    const std::size_t          from    = samples.size() / 4;
    const std::size_t          to      = samples.size() * 3 / 4;

    double sum = 0.0;
    for (std::size_t frame = from; frame < to; ++frame)
        sum += static_cast<double>(samples[frame]) * static_cast<double>(samples[frame]);
    return std::sqrt(sum / static_cast<double>(to - from));
}

/// A silence bed with short decaying bursts every quarter second.
AudioFileData clickTrain(int clicks)
{
    AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = 1;
    data.frameCount   = static_cast<FrameCount>(sampleRate * 0.25 * (clicks + 1));
    data.channels.assign(1, std::vector<Sample>(static_cast<std::size_t>(data.frameCount), 0.0f));

    for (int click = 0; click < clicks; ++click) {
        const auto at = static_cast<std::size_t>(sampleRate * 0.25 * (click + 1));
        for (std::size_t frame = 0; frame < 240 && at + frame < data.channels[0].size(); ++frame)
            data.channels[0][at + frame] =
                static_cast<Sample>(0.9 * std::exp(-static_cast<double>(frame) / 40.0));
    }

    return data;
}

/// Counts distinct bursts: excursions above the threshold separated by at
/// least 50 ms of quiet.
int countBursts(const AudioFileData& data, float threshold = 0.2f)
{
    const auto quietGap = static_cast<std::size_t>(data.sampleRate * 0.05);

    int         bursts     = 0;
    std::size_t quietFor   = quietGap;

    for (const Sample sample : data.channels[0]) {
        if (std::fabs(sample) > threshold) {
            if (quietFor >= quietGap)
                ++bursts;
            quietFor = 0;
        } else {
            ++quietFor;
        }
    }

    return bursts;
}

} // namespace

TEST_CASE("ratio one and zero semitones is the exact identity")
{
    const AudioFileData input  = sineData(440.0, 1.0);
    const AudioFileData output = timeStretch(input, {});

    REQUIRE(output.frameCount == input.frameCount);
    for (std::size_t channel = 0; channel < 2; ++channel)
        for (std::size_t frame = 0; frame < input.channels[channel].size(); ++frame)
            REQUIRE(output.channels[channel][frame] == input.channels[channel][frame]);
}

TEST_CASE("stretching doubles the duration and keeps the pitch")
{
    const AudioFileData input = sineData(440.0, 1.0);

    StretchOptions options;
    options.ratio = 2.0;
    const AudioFileData output = timeStretch(input, options);

    CHECK(static_cast<double>(output.frameCount)
          == doctest::Approx(static_cast<double>(input.frameCount) * 2.0).epsilon(0.01));
    CHECK(dominantFrequency(output) == doctest::Approx(440.0).epsilon(0.01));
    CHECK(rmsOfMiddle(output) == doctest::Approx(rmsOfMiddle(input)).epsilon(0.1));
}

TEST_CASE("compressing halves the duration and keeps the pitch")
{
    const AudioFileData input = sineData(330.0, 1.0);

    StretchOptions options;
    options.ratio = 0.5;
    const AudioFileData output = timeStretch(input, options);

    CHECK(static_cast<double>(output.frameCount)
          == doctest::Approx(static_cast<double>(input.frameCount) * 0.5).epsilon(0.01));
    CHECK(dominantFrequency(output) == doctest::Approx(330.0).epsilon(0.01));
}

TEST_CASE("a pitch shift moves the frequency and keeps the duration")
{
    const AudioFileData input = sineData(440.0, 1.0);

    StretchOptions up;
    up.pitchSemitones = 12.0;
    const AudioFileData octave = timeStretch(input, up);

    CHECK(static_cast<double>(octave.frameCount)
          == doctest::Approx(static_cast<double>(input.frameCount)).epsilon(0.01));
    CHECK(dominantFrequency(octave) == doctest::Approx(880.0).epsilon(0.02));

    StretchOptions down;
    down.pitchSemitones = -12.0;
    const AudioFileData lower = timeStretch(input, down);

    CHECK(static_cast<double>(lower.frameCount)
          == doctest::Approx(static_cast<double>(input.frameCount)).epsilon(0.01));
    CHECK(dominantFrequency(lower) == doctest::Approx(220.0).epsilon(0.02));
}

TEST_CASE("stretch and pitch combine independently")
{
    const AudioFileData input = sineData(440.0, 1.0);

    StretchOptions options;
    options.ratio          = 1.5;
    options.pitchSemitones = -5.0;
    const AudioFileData output = timeStretch(input, options);

    CHECK(static_cast<double>(output.frameCount)
          == doctest::Approx(static_cast<double>(input.frameCount) * 1.5).epsilon(0.01));
    CHECK(dominantFrequency(output)
          == doctest::Approx(440.0 * std::pow(2.0, -5.0 / 12.0)).epsilon(0.02));
}

TEST_CASE("transient locking keeps every click exactly once through a slow stretch")
{
    const AudioFileData input = clickTrain(6);
    REQUIRE(countBursts(input) == 6);

    StretchOptions slow;
    slow.ratio = 2.0;
    const AudioFileData stretched = timeStretch(input, slow);
    CHECK(countBursts(stretched) == 6);

    StretchOptions fast;
    fast.ratio = 0.5;
    const AudioFileData compressed = timeStretch(input, fast);
    CHECK(countBursts(compressed) == 6);
}

TEST_CASE("stereo phase survives a stretch")
{
    // Identical channels in, identical channels out — alignment decisions are
    // shared, never per channel.
    const AudioFileData input = sineData(200.0, 0.5);

    StretchOptions options;
    options.ratio = 1.7;
    const AudioFileData output = timeStretch(input, options);

    for (std::size_t frame = 0; frame < output.channels[0].size(); ++frame)
        REQUIRE(output.channels[0][frame] == output.channels[1][frame]);
}

// ── Integration ───────────────────────────────────────────────────────────────

namespace {

struct ScratchDirectory {
    explicit ScratchDirectory(const std::string& name)
        : path(fs::temp_directory_path()
               / ("incdaw-test-" + name + "-" + std::to_string(nextSerial())))
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDirectory()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }

    fs::path path;

private:
    static int nextSerial()
    {
        static std::atomic<int> counter{0};
        return ++counter;
    }
};

} // namespace

TEST_CASE("stretching an asset rewrites the file undoably")
{
    ScratchDirectory scratch("stretch-asset");
    const fs::path file = scratch.path / "tone.wav";

    const AudioFileData original = sineData(440.0, 1.0);
    REQUIRE(engine::WavFile::write(file.string(), original));

    incdaw::project::Project project;
    incdaw::app::CommandRegistry registry { project };

    incdaw::project::AudioAsset& asset = project.addAudioAsset(file.string());
    asset.absolutePath = file.string();
    asset.sampleRate   = sampleRate;
    asset.frameCount   = original.frameCount;
    asset.channelCount = 2;

    REQUIRE(registry.execute(std::make_unique<incdaw::app::StretchAssetCommand>(
        asset.id, engine::edits::Region{0, original.frameCount}, 2.0, 0.0)));

    AudioFileData stretched;
    REQUIRE(engine::WavFile::read(file.string(), stretched));
    CHECK(static_cast<double>(stretched.frameCount)
          == doctest::Approx(static_cast<double>(original.frameCount) * 2.0).epsilon(0.01));
    CHECK(dominantFrequency(stretched) == doctest::Approx(440.0).epsilon(0.01));
    CHECK(project.audioAssets()[0].frameCount == stretched.frameCount);

    // Undo restores the file bit-exactly.
    CHECK(registry.undo());

    AudioFileData restored;
    REQUIRE(engine::WavFile::read(file.string(), restored));
    REQUIRE(restored.frameCount == original.frameCount);
    for (std::size_t channel = 0; channel < 2; ++channel)
        for (std::size_t frame = 0; frame < restored.channels[channel].size(); ++frame)
            REQUIRE(restored.channels[channel][frame] == original.channels[channel][frame]);

    CHECK(registry.redo());
    AudioFileData again;
    REQUIRE(engine::WavFile::read(file.string(), again));
    CHECK(again.frameCount == stretched.frameCount);
}

TEST_CASE("stretch-resizing a clip scales its ratio with its length")
{
    incdaw::project::Project project;
    incdaw::app::CommandRegistry registry { project };

    incdaw::project::AudioAsset& asset = project.addAudioAsset("media/take.wav");
    asset.sampleRate = sampleRate;
    asset.frameCount = 96000;

    const incdaw::project::EntityId track =
        project.addTrack(incdaw::project::TrackType::audio, "Audio 1").id;

    incdaw::project::Clip& clip =
        project.addClip(incdaw::project::ClipType::audio, track, asset.id);
    clip.start  = 0;
    clip.length = 48000;   // one beat at 120 BPM / 48 kHz is 24000 frames
    const incdaw::project::EntityId clipId = clip.id;

    // Two beats longer: 48000 + 48000 = 96000 frames, ratio doubles.
    REQUIRE(registry.executeMerging(std::make_unique<incdaw::app::StretchClipsCommand>(
        incdaw::app::ClipIds{ clipId }, incdaw::engine::ticksPerQuarterNote * 2)));

    CHECK(project.findClip(clipId)->length == 96000);
    CHECK(project.findClip(clipId)->stretchRatio == doctest::Approx(2.0));

    REQUIRE(registry.executeMerging(std::make_unique<incdaw::app::StretchClipsCommand>(
        incdaw::app::ClipIds{ clipId }, -incdaw::engine::ticksPerQuarterNote)));
    CHECK(project.findClip(clipId)->length == 72000);
    CHECK(project.findClip(clipId)->stretchRatio == doctest::Approx(1.5));

    // One merged gesture: a single undo returns to the unstretched clip.
    CHECK(registry.undo());
    CHECK(project.findClip(clipId)->length == 48000);
    CHECK(project.findClip(clipId)->stretchRatio == doctest::Approx(1.0));
    CHECK_FALSE(registry.canUndo());
}

TEST_CASE("a warped clip plays its stretched audio through the compiled graph")
{
    ScratchDirectory scratch("warped-clip");
    const fs::path file = scratch.path / "tone.wav";

    const AudioFileData original = sineData(440.0, 1.0, 0.5);
    REQUIRE(engine::WavFile::write(file.string(), original));

    incdaw::project::Project project;
    incdaw::engine::TempoMap tempo;
    tempo.setSampleRate(sampleRate);

    incdaw::project::AudioAsset& asset = project.addAudioAsset(file.string());
    asset.absolutePath = file.string();
    asset.sampleRate   = sampleRate;
    asset.frameCount   = original.frameCount;
    asset.channelCount = 2;

    const incdaw::project::EntityId track =
        project.addTrack(incdaw::project::TrackType::audio, "Audio 1").id;

    incdaw::project::Clip& clip =
        project.addClip(incdaw::project::ClipType::audio, track, asset.id);
    clip.start        = 0;
    clip.length       = 96000;   // twice the source: stretched, not repeated
    clip.stretchRatio = 2.0;

    incdaw::project::GraphCompileOptions options;
    options.source     = incdaw::project::PlaybackSource::arrangement;
    options.masterGain = engine::Sample{1.0f};

    auto compiled = incdaw::project::compileProjectGraph(project, tempo, options);
    REQUIRE(compiled);

    // Render 1.5 s: inside the stretched clip, past the original's length.
    engine::AudioBufferPool pool;
    pool.allocate(1, 2, 256);
    const engine::AudioBufferView view = pool.buffer(0);

    AudioFileData rendered;
    rendered.sampleRate   = sampleRate;
    rendered.channelCount = 1;
    rendered.frameCount   = 72000;
    rendered.channels.assign(1, std::vector<engine::Sample>(72000));

    for (engine::FramePosition position = 0; position < 72000; position += 256) {
        compiled.graph->process(view, 256, position);
        for (std::size_t frame = 0; frame < 256 && position + static_cast<long long>(frame) < 72000;
             ++frame)
            rendered.channels[0][static_cast<std::size_t>(position) + frame] =
                view.channel(0)[frame];
    }

    // The second half of the render sits beyond the source's own length —
    // only a stretch can put audio there — and the pitch is still 440.
    double tail = 0.0;
    for (std::size_t frame = 50000; frame < 70000; ++frame)
        tail += static_cast<double>(rendered.channels[0][frame])
              * static_cast<double>(rendered.channels[0][frame]);
    CHECK(std::sqrt(tail / 20000.0) > 0.05);

    CHECK(dominantFrequency(rendered) == doctest::Approx(440.0).epsilon(0.02));
}
