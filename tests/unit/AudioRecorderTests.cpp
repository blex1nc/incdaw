// Phase 12 (part 2) — recording.
//
// The load-bearing tests are the two loopback cases at the bottom: recorded
// audio must land sample-accurately against the source when the reported
// device latency is applied, and must land exactly `latency` frames late when
// it is not. Together they prove the compensation is real, the same way the
// PDC test proves delay compensation by asserting its own absence fails.

#include "doctest.h"

#include "engine/AudioEngine.h"
#include "engine/audio/AudioRecorder.h"
#include "engine/audio/WavFile.h"
#include "engine/core/RealtimeGuard.h"
#include "engine/core/SampleRingBuffer.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <thread>
#include <vector>

using namespace incdaw::engine;
namespace fs = std::filesystem;

namespace {

struct ScratchFile {
    fs::path path;

    explicit ScratchFile(const char* name)
        : path(fs::temp_directory_path() / name)
    {
        std::error_code code;
        fs::remove(path, code);
    }

    ~ScratchFile()
    {
        std::error_code code;
        fs::remove(path, code);
    }
};

/// A capture-stream sample that is never zero, so silence and signal cannot
/// be confused, and never repeats within a take, so a shifted comparison
/// cannot accidentally match.
Sample sourceSample(FrameCount frame)
{
    return static_cast<Sample>(0.25 + 0.5 * std::sin(2.0 * 3.14159265358979 * 431.0
                                                     * static_cast<double>(frame) / 48000.0)
                               + 1.0e-6 * static_cast<double>(frame));
}

} // namespace

// ── SampleRingBuffer ─────────────────────────────────────────────────────────

TEST_CASE("sample ring preserves content across wrap-around")
{
    SampleRingBuffer ring;
    ring.reset(100);   // rounds up to 128; capacity 127

    REQUIRE(ring.capacity() == 127);

    std::vector<Sample> out(300);
    FrameCount sequence = 0;
    std::size_t readTotal = 0;

    // Repeatedly write 60 and read 60, forcing many wraps.
    for (int cycle = 0; cycle < 40; ++cycle) {
        std::vector<Sample> chunk(60);
        for (auto& value : chunk)
            value = static_cast<Sample>(sequence++);

        REQUIRE(ring.write(chunk.data(), chunk.size()) == chunk.size());
        REQUIRE(ring.read(out.data() + 0, 60) == 60);

        for (std::size_t index = 0; index < 60; ++index)
            REQUIRE(out[index] == static_cast<Sample>(readTotal + index));

        readTotal += 60;
    }
}

TEST_CASE("sample ring refuses to overwrite unread data")
{
    SampleRingBuffer ring;
    ring.reset(100);   // capacity 127

    std::vector<Sample> chunk(127, 1.0f);
    REQUIRE(ring.write(chunk.data(), chunk.size()) == 127);
    CHECK(ring.freeSpace() == 0);

    // Full: nothing more fits, and what was written is intact.
    CHECK(ring.write(chunk.data(), 10) == 0);

    std::vector<Sample> out(127);
    REQUIRE(ring.read(out.data(), out.size()) == 127);
    for (const auto value : out)
        CHECK(value == 1.0f);
}

// ── AudioRecorder ────────────────────────────────────────────────────────────

TEST_CASE("recorder round trip: what was captured is what is on disk")
{
    ScratchFile scratch{"incdaw-recorder-roundtrip.wav"};

    AudioRecorder recorder;

    AudioRecorder::Options options;
    options.sampleRate   = 48000.0;
    options.channelCount = 2;
    options.maxBlockSize = 512;

    REQUIRE(bool(recorder.start(scratch.path, options)));
    REQUIRE(recorder.isRecording());

    constexpr FrameCount total = 4801;   // deliberately not a block multiple

    std::vector<Sample> left(512), right(512);
    for (FrameCount position = 0; position < total;) {
        const FrameCount frames = std::min<FrameCount>(512, total - position);

        for (FrameCount index = 0; index < frames; ++index) {
            left[static_cast<std::size_t>(index)]  = sourceSample(position + index);
            right[static_cast<std::size_t>(index)] = -sourceSample(position + index);
        }

        const Sample* channels[] = {left.data(), right.data()};
        recorder.captureAudioBlock(channels, 2, frames,
                                   1'000'000'000ull + static_cast<std::uint64_t>(position) * 20833ull);
        position += frames;
    }

    const auto take = recorder.stop();
    REQUIRE(bool(take));
    CHECK(take.droppedFrames == 0);
    REQUIRE(take.frameCount == total);

    AudioFileData loaded;
    REQUIRE(bool(WavFile::read(scratch.path, loaded)));
    REQUIRE(loaded.frameCount == total);
    REQUIRE(loaded.channelCount == 2);

    for (FrameCount frame = 0; frame < total; ++frame) {
        REQUIRE(loaded.channels[0][static_cast<std::size_t>(frame)] == sourceSample(frame));
        REQUIRE(loaded.channels[1][static_cast<std::size_t>(frame)] == -sourceSample(frame));
    }
}

TEST_CASE("capture path is allocation-free")
{
    if (!rt::guardEnabled()) {
        MESSAGE("realtime guard not compiled in; not verified");
        return;
    }

    ScratchFile scratch{"incdaw-recorder-rtsafe.wav"};

    AudioRecorder recorder;

    AudioRecorder::Options options;
    options.sampleRate   = 48000.0;
    options.channelCount = 2;
    options.maxBlockSize = 512;

    REQUIRE(bool(recorder.start(scratch.path, options)));

    std::vector<Sample> left(512, 0.1f), right(512, -0.1f);
    const Sample* channels[] = {left.data(), right.data()};

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext realtimeScope;

        for (int block = 0; block < 32; ++block)
            recorder.captureAudioBlock(channels, 2, 512,
                                       1'000'000'000ull + static_cast<std::uint64_t>(block));
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);

    REQUIRE(bool(recorder.stop()));
}

TEST_CASE("a full ring drops whole frames and reports them honestly")
{
    ScratchFile scratch{"incdaw-recorder-overflow.wav"};

    AudioRecorder recorder;

    AudioRecorder::Options options;
    options.sampleRate   = 48000.0;
    options.channelCount = 1;
    options.maxBlockSize = 512;
    options.ringSeconds  = 0.005;   // 240 samples requested -> ring capacity 255

    REQUIRE(bool(recorder.start(scratch.path, options)));

    // One block larger than the whole ring, delivered before the writer can
    // drain anything: exactly capacity frames fit, the rest must be counted.
    std::vector<Sample> block(512);
    for (FrameCount index = 0; index < 512; ++index)
        block[static_cast<std::size_t>(index)] = sourceSample(index);

    const Sample* channels[] = {block.data()};
    recorder.captureAudioBlock(channels, 1, 512, 1'000'000'000ull);

    const auto take = recorder.stop();
    REQUIRE(bool(take));
    CHECK(take.frameCount == 255);
    CHECK(take.droppedFrames == 257);
}

TEST_CASE("engine forwards capture to the installed sink and only then")
{
    ScratchFile scratch{"incdaw-recorder-engine.wav"};

    AudioEngine   engine;
    AudioRecorder recorder;

    AudioRecorder::Options options;
    options.sampleRate   = 48000.0;
    options.channelCount = 1;
    options.maxBlockSize = 512;

    REQUIRE(bool(recorder.start(scratch.path, options)));

    std::vector<Sample> block(512, 0.5f);
    const Sample* channels[] = {block.data()};

    // Through the platform-facing interface, exactly as a device would call it.
    incdaw::platform::AudioIOCallback& callback = engine;

    callback.captureAudioBlock(channels, 1, 512, 1'000'000'000ull);
    CHECK(recorder.capturedFrames() == 0);   // no sink installed yet

    engine.setCaptureSink(&recorder);
    callback.captureAudioBlock(channels, 1, 512, 1'000'000'000ull);
    CHECK(recorder.capturedFrames() == 512);

    engine.setCaptureSink(nullptr);
    callback.captureAudioBlock(channels, 1, 512, 1'000'000'000ull);
    CHECK(recorder.capturedFrames() == 512);

    REQUIRE(bool(recorder.stop()));
}

// ── The Phase 12 exit criterion ──────────────────────────────────────────────
//
// Simulated loopback. The source's first frame leaves the output at host time
// T0. The capture stream contains silence (the recorder armed early), then
// the source; the simulated device delivers each block stamped `latency`
// frames late, exactly as a real device's uncompensated input timestamps are.
// The recorder is told that latency, and the take's reported start must put
// the source back at T0 to the sample.

namespace {

struct LoopbackResult {
    AudioRecorder::Take take;
    FrameCount          silenceFrames = 0;
    std::uint64_t       sourceStartNanos = 0;   ///< T0
    double              sampleRate = 0.0;
};

LoopbackResult runLoopback(const fs::path& path, FrameCount compensatedLatency,
                           FrameCount deviceLatency)
{
    constexpr double     rate          = 48000.0;
    constexpr FrameCount blockSize     = 512;
    constexpr FrameCount silenceFrames = 480;
    constexpr FrameCount sourceFrames  = 4096;
    constexpr std::uint64_t T0         = 1'000'000'000'000ull;

    AudioRecorder recorder;

    AudioRecorder::Options options;
    options.sampleRate    = rate;
    options.channelCount  = 1;
    options.maxBlockSize  = blockSize;
    options.latencyFrames = compensatedLatency;

    REQUIRE(bool(recorder.start(path, options)));

    const double nanosPerFrame = 1.0e9 / rate;

    constexpr FrameCount total = silenceFrames + sourceFrames;
    std::vector<Sample> block(blockSize);

    for (FrameCount position = 0; position < total;) {
        const FrameCount frames = std::min<FrameCount>(blockSize, total - position);

        for (FrameCount index = 0; index < frames; ++index) {
            const FrameCount stream = position + index;
            block[static_cast<std::size_t>(index)] =
                stream < silenceFrames ? 0.0f : sourceSample(stream - silenceFrames);
        }

        // The event time of this block's first frame, plus the device's lag.
        const auto offset = static_cast<double>(position - silenceFrames + deviceLatency);
        const auto stamp  = static_cast<std::uint64_t>(
            static_cast<std::int64_t>(T0) + std::llround(offset * nanosPerFrame));

        const Sample* channels[] = {block.data()};
        recorder.captureAudioBlock(channels, 1, frames, stamp);
        position += frames;
    }

    LoopbackResult result;
    result.take             = recorder.stop();
    result.silenceFrames    = silenceFrames;
    result.sourceStartNanos = T0;
    result.sampleRate       = rate;

    REQUIRE(bool(result.take));
    REQUIRE(result.take.droppedFrames == 0);
    REQUIRE(result.take.frameCount == total);

    // Content integrity first: the file holds the silence then the source,
    // bit-exactly. Alignment claims mean nothing if the audio is wrong.
    AudioFileData loaded;
    REQUIRE(bool(WavFile::read(path, loaded)));
    REQUIRE(loaded.frameCount == total);

    for (FrameCount frame = 0; frame < total; ++frame) {
        const Sample expected =
            frame < silenceFrames ? 0.0f : sourceSample(frame - silenceFrames);
        REQUIRE(loaded.channels[0][static_cast<std::size_t>(frame)] == expected);
    }

    return result;
}

/// Where the source's first frame lands, in frames, relative to where it was
/// actually played. Zero is sample-accurate.
FrameCount alignmentErrorFrames(const LoopbackResult& result)
{
    const double nanosPerFrame = 1.0e9 / result.sampleRate;

    const double sourceRecordedAt = static_cast<double>(result.take.startHostTimeNanos)
                                  + static_cast<double>(result.silenceFrames) * nanosPerFrame;

    return std::llround((sourceRecordedAt - static_cast<double>(result.sourceStartNanos))
                        / nanosPerFrame);
}

} // namespace

TEST_CASE("EXIT CRITERION: recorded audio lands sample-accurately against the source")
{
    ScratchFile scratch{"incdaw-loopback-compensated.wav"};

    // The recorder is told the same latency the simulated device imposes —
    // the number a real device reports as totalInputLatencyFrames.
    const auto result = runLoopback(scratch.path, 333, 333);

    CHECK(alignmentErrorFrames(result) == 0);
}

TEST_CASE("EXIT CRITERION: without compensation the same capture lands exactly the latency late")
{
    ScratchFile scratch{"incdaw-loopback-uncompensated.wav"};

    // Same device lag, compensation switched off: the take must misalign by
    // exactly that many frames. This is what proves the compensated case
    // passes because of the subtraction, not by accident.
    const auto result = runLoopback(scratch.path, 0, 333);

    CHECK(alignmentErrorFrames(result) == 333);
}
