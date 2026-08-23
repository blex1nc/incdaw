// C9 — the input-side pre-record buffer.
//
// The take nobody armed: the pass that was meant as a rehearsal and was better
// than the three that followed it. Mechanically the master Audio Logger's
// circle, pointed at the input instead — and a SECOND instance of it rather
// than a mode on the first, because one of the two is a microphone.

#include "doctest.h"

#include "app/AppSettings.h"
#include "engine/AudioEngine.h"
#include "engine/audio/AudioLogger.h"
#include "engine/core/RealtimeGuard.h"
#include "project/RecordingSession.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace fs = std::filesystem;

namespace {

struct ScratchDir {
    explicit ScratchDir(const std::string& name)
        : path(fs::temp_directory_path() / (name + "-" + std::to_string(nextSerial())))
    {
        std::error_code code;
        fs::remove_all(path, code);
        fs::create_directories(path, code);
    }

    ~ScratchDir()
    {
        std::error_code code;
        fs::remove_all(path, code);
    }

    fs::path path;

private:
    static int nextSerial()
    {
        static int serial = 0;
        return ++serial;
    }
};

/// Feeds `frames` of stereo input whose value encodes the frame index.
void feed(AudioLogger& logger, FrameCount& counter, FrameCount frames)
{
    std::vector<Sample> left(static_cast<std::size_t>(frames));
    std::vector<Sample> right(static_cast<std::size_t>(frames));

    for (FrameCount frame = 0; frame < frames; ++frame) {
        left[static_cast<std::size_t>(frame)]  = static_cast<Sample>(counter + frame) * 1.0e-6f;
        right[static_cast<std::size_t>(frame)] = -left[static_cast<std::size_t>(frame)];
    }

    const Sample* channels[] = {left.data(), right.data()};
    logger.log(channels, 2, frames);
    counter += frames;
}

} // namespace

// ── The buffer ───────────────────────────────────────────────────────────────

TEST_CASE("the input buffer is a separate switch from the master logger")
{
    AudioEngine engine;

    // Neither is on, and turning one on does not turn the other on. A buffer
    // that quietly became the room because playback logging was enabled would
    // be a privacy bug, not a convenience.
    CHECK_FALSE(engine.logger().isEnabled());
    CHECK_FALSE(engine.inputLogger().isEnabled());

    engine.logger().setEnabled(true);
    CHECK_FALSE(engine.inputLogger().isEnabled());

    engine.inputLogger().setEnabled(true);
    engine.logger().setEnabled(false);
    CHECK(engine.inputLogger().isEnabled());
}

TEST_CASE("the input buffer keeps the freshest window, in order")
{
    AudioLogger logger;
    logger.prepare(48000.0, 2, 0.25);   // 12000 frames
    logger.setEnabled(true);

    FrameCount counter = 0;

    // Well past the capacity, so what comes back is whatever survived the
    // wrapping rather than the beginning.
    for (int block = 0; block < 60; ++block)
        feed(logger, counter, 512);

    AudioFileData grabbed;
    const FrameCount frames = logger.grab(grabbed);

    REQUIRE(frames > 0);
    REQUIRE(grabbed.channelCount == 2);
    CHECK(grabbed.sampleRate == doctest::Approx(48000.0));

    // Oldest first, and contiguous: each frame is exactly one more than the
    // last, which a torn or misordered window cannot be.
    for (FrameCount frame = 1; frame < frames; ++frame) {
        const auto index = static_cast<std::size_t>(frame);
        CHECK(grabbed.channels[0][index]
              == doctest::Approx(grabbed.channels[0][index - 1] + 1.0e-6f).epsilon(0.001));
    }

    // And it is the END of what was fed, not the start.
    const auto last = static_cast<std::size_t>(frames - 1);
    CHECK(grabbed.channels[0][last]
          == doctest::Approx(static_cast<Sample>(counter - 1) * 1.0e-6f).epsilon(0.001));
}

TEST_CASE("a disabled input buffer keeps nothing")
{
    AudioLogger logger;
    logger.prepare(48000.0, 2, 1.0);

    FrameCount counter = 0;
    for (int block = 0; block < 20; ++block)
        feed(logger, counter, 512);

    AudioFileData grabbed;
    CHECK(logger.grab(grabbed) == 0);
}

TEST_CASE("feeding the input buffer allocates nothing on the capture thread")
{
    AudioLogger logger;
    logger.prepare(48000.0, 2, 1.0);
    logger.setEnabled(true);

    FrameCount counter = 0;

    // The blocks are built outside the guarded scope; what is measured is the
    // logging, which runs on the input device's realtime thread.
    std::vector<Sample> left(512), right(512);
    const Sample* channels[] = {left.data(), right.data()};

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        for (int block = 0; block < 64; ++block)
            logger.log(channels, 2, 512);
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);

    (void)counter;

    if (!rt::guardEnabled())
        MESSAGE("realtime guard disabled in this build — allocation not verified");
}

// ── Keeping it as a take ─────────────────────────────────────────────────────

TEST_CASE("keeping a buffer that is off is refused rather than silently empty")
{
    ScratchDir scratch{"incdaw-preroll-off"};

    AudioEngine              engine;
    project::RecordingSession session;

    const auto placement = session.keepPreRoll(engine, scratch.path);

    CHECK_FALSE(placement);
    CHECK_FALSE(placement.error.empty());

    // Nothing was written: a refusal that leaves a stray file behind is worse
    // than one that does not.
    CHECK(fs::is_empty(scratch.path));
}

TEST_CASE("keeping an empty buffer is refused")
{
    ScratchDir scratch{"incdaw-preroll-empty"};

    AudioEngine engine;
    engine.inputLogger().prepare(48000.0, 2, 1.0);
    engine.inputLogger().setEnabled(true);

    project::RecordingSession session;
    const auto placement = session.keepPreRoll(engine, scratch.path);

    CHECK_FALSE(placement);
    CHECK(fs::is_empty(scratch.path));
}

TEST_CASE("what the input was doing lands as a take ending at the playhead")
{
    ScratchDir scratch{"incdaw-preroll"};

    AudioEngine engine;
    engine.inputLogger().prepare(48000.0, 2, 2.0);
    engine.inputLogger().setEnabled(true);

    FrameCount counter = 0;
    for (int block = 0; block < 80; ++block)
        feed(engine.inputLogger(), counter, 512);

    // The playhead the user was listening against.
    engine.transport().seek(240000);
    {
        BlockSegment plan[Transport::maxSegmentsPerBlock];
        (void)engine.transport().processBlock(512, plan, Transport::maxSegmentsPerBlock);
    }

    project::RecordingSession session;
    const auto placement = session.keepPreRoll(engine, scratch.path);

    REQUIRE(placement);
    CHECK(placement.channelCount == 2);
    CHECK(placement.frameCount == counter);
    CHECK(fs::exists(placement.path));

    // The sound just played lands under the playhead it was played against.
    // Anywhere else and the feature is worse than not having it.
    CHECK(placement.startFrame + placement.frameCount == engine.transport().position());

    AudioFileData written;
    REQUIRE(bool(WavFile::read(placement.path, written)));
    CHECK(written.frameCount == placement.frameCount);
    CHECK(written.channelCount == 2);
}

TEST_CASE("a take that would start before zero is clamped, not placed before time")
{
    ScratchDir scratch{"incdaw-preroll-clamp"};

    AudioEngine engine;
    engine.inputLogger().prepare(48000.0, 1, 2.0);
    engine.inputLogger().setEnabled(true);

    std::vector<Sample> block(4096, 0.5f);
    const Sample*       channels[] = {block.data()};

    for (int index = 0; index < 20; ++index)
        engine.inputLogger().log(channels, 1, 4096);

    // The playhead is near the top of the song; the window is longer than
    // what came before it.
    engine.transport().seek(1000);
    {
        BlockSegment plan[Transport::maxSegmentsPerBlock];
        (void)engine.transport().processBlock(512, plan, Transport::maxSegmentsPerBlock);
    }

    project::RecordingSession session;
    const auto placement = session.keepPreRoll(engine, scratch.path);

    REQUIRE(placement);
    CHECK(placement.startFrame == 0);
}

TEST_CASE("the window can be trimmed to the most recent seconds")
{
    ScratchDir scratch{"incdaw-preroll-trim"};

    AudioEngine engine;
    engine.inputLogger().prepare(48000.0, 1, 4.0);
    engine.inputLogger().setEnabled(true);

    std::vector<Sample> block(4800);
    for (std::size_t index = 0; index < block.size(); ++index)
        block[index] = static_cast<Sample>(index) * 1.0e-5f;

    const Sample* channels[] = {block.data()};
    for (int index = 0; index < 30; ++index)
        engine.inputLogger().log(channels, 1, 4800);

    project::RecordingSession session;
    const auto placement = session.keepPreRoll(engine, scratch.path, 0.5);

    REQUIRE(placement);

    // Half a second at 48 kHz. The interesting part of a buffer nobody armed
    // is always the part that just happened, so the trim keeps the END.
    CHECK(placement.frameCount == 24000);
}

// ── The preference ───────────────────────────────────────────────────────────

TEST_CASE("the pre-record preference round-trips and defaults to off")
{
    app::AppSettings settings;
    CHECK_FALSE(settings.inputPreRecordEnabled);

    settings.inputPreRecordEnabled = true;
    CHECK(app::AppSettings::fromJson(settings.toJson()).inputPreRecordEnabled);

    // A settings file written before this existed keeps the microphone shut.
    CHECK_FALSE(app::AppSettings::fromJson(R"({"audio":{"sampleRate":48000}})")
                    .inputPreRecordEnabled);
}
