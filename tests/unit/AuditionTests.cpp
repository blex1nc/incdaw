// FL2026 P9 (part 3) — the Browser's preview.
//
// A preview is audio the project does not know about: it must sound with the
// transport stopped, sum into the same block the graph just wrote, stop by
// itself at the end of the file, and never allocate on the audio thread. The
// hand-over of the decoded buffer is the delicate part — the audio thread
// holds a raw pointer, so the buffer may only be freed once the block counter
// has moved past it.

#include "doctest.h"

#include "engine/audio/AuditionPlayer.h"
#include "engine/core/RealtimeGuard.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;

namespace {

std::shared_ptr<AudioFileData> makeFile(std::size_t channels, FrameCount frames, SampleRate rate,
                                        Sample value = 0.5F)
{
    auto data          = std::make_shared<AudioFileData>();
    data->sampleRate   = rate;
    data->channelCount = channels;
    data->frameCount   = frames;
    data->channels.assign(channels, std::vector<Sample>(static_cast<std::size_t>(frames)));

    for (std::size_t channel = 0; channel < channels; ++channel)
        for (FrameCount frame = 0; frame < frames; ++frame)
            data->channels[channel][static_cast<std::size_t>(frame)] =
                value * static_cast<Sample>(channel + 1);

    return data;
}

/// A block of output the player can sum into, laid out the way the device's
/// buffers are: separate contiguous channels.
struct Block {
    std::vector<std::vector<Sample>> storage;
    std::vector<Sample*>             pointers;

    Block(std::size_t channels, FrameCount frames)
        : storage(channels, std::vector<Sample>(static_cast<std::size_t>(frames), Sample{0}))
    {
        for (std::vector<Sample>& channel : storage)
            pointers.push_back(channel.data());
    }

    [[nodiscard]] AudioBufferView view(FrameCount frames)
    {
        return AudioBufferView{pointers.data(), pointers.size(), frames};
    }

    void silence()
    {
        for (std::vector<Sample>& channel : storage)
            std::fill(channel.begin(), channel.end(), Sample{0});
    }
};

} // namespace

TEST_CASE("a preview sums into the block and stops at the end of the file")
{
    AuditionPlayer player;
    Block          block(2, 64);

    CHECK_FALSE(player.isPlaying());

    // 100 frames at the device rate: two full blocks and a partial third.
    player.play(makeFile(2, 100, 48000.0), 48000.0, 1.0F, 0);
    CHECK(player.isPlaying());

    player.render(block.view(64));

    CHECK(block.storage[0][0] == doctest::Approx(0.5));
    CHECK(block.storage[1][0] == doctest::Approx(1.0));   // second channel is twice as loud
    CHECK(block.storage[0][63] == doctest::Approx(0.5));
    CHECK(player.isPlaying());

    block.silence();
    player.render(block.view(64));

    // Frames 64..99 play; the file ends inside the block and the rest is silent.
    CHECK(block.storage[0][35] == doctest::Approx(0.5));
    CHECK(block.storage[0][36] == doctest::Approx(0.0));
    CHECK_FALSE(player.isPlaying());

    // A finished preview adds nothing further.
    block.silence();
    player.render(block.view(64));
    CHECK(block.storage[0][0] == doctest::Approx(0.0));
}

TEST_CASE("a preview adds to what the graph wrote rather than replacing it")
{
    AuditionPlayer player;
    Block          block(2, 16);

    for (std::vector<Sample>& channel : block.storage)
        std::fill(channel.begin(), channel.end(), Sample{0.25F});

    player.play(makeFile(1, 32, 48000.0, 0.5F), 48000.0, 1.0F, 0);
    player.render(block.view(16));

    // Mono file, so both output channels get the same preview on top of the mix.
    CHECK(block.storage[0][0] == doctest::Approx(0.75));
    CHECK(block.storage[1][0] == doctest::Approx(0.75));
}

TEST_CASE("a file at another sample rate is repitched, not refused")
{
    AuditionPlayer player;
    Block          block(1, 64);

    // 24 kHz source into a 48 kHz device: half speed, so 32 source frames
    // occupy 64 output frames.
    player.play(makeFile(1, 32, 24000.0), 48000.0, 1.0F, 0);
    player.render(block.view(64));

    CHECK(block.storage[0][62] == doctest::Approx(0.5));
    CHECK_FALSE(player.isPlaying());

    // And the ramp between two frames is interpolated, not stepped.
    auto ramp = std::make_shared<AudioFileData>();
    ramp->sampleRate   = 24000.0;
    ramp->channelCount = 1;
    ramp->frameCount   = 4;
    ramp->channels     = {{0.0F, 1.0F, 0.0F, 0.0F}};

    Block ramps(1, 8);
    player.play(ramp, 48000.0, 1.0F, 0);
    player.render(ramps.view(8));

    CHECK(ramps.storage[0][0] == doctest::Approx(0.0));
    CHECK(ramps.storage[0][1] == doctest::Approx(0.5));
    CHECK(ramps.storage[0][2] == doctest::Approx(1.0));
}

TEST_CASE("stop silences the preview on the next block")
{
    AuditionPlayer player;
    Block          block(2, 32);

    player.play(makeFile(2, 4800, 48000.0), 48000.0, 1.0F, 0);
    player.render(block.view(32));
    CHECK(block.storage[0][0] == doctest::Approx(0.5));

    player.stop();
    block.silence();
    player.render(block.view(32));

    CHECK(block.storage[0][0] == doctest::Approx(0.0));
    CHECK_FALSE(player.isPlaying());
}

TEST_CASE("a second preview restarts from the top of the new file")
{
    AuditionPlayer player;
    Block          block(1, 8);

    player.play(makeFile(1, 64, 48000.0, 0.5F), 48000.0, 1.0F, 0);
    player.render(block.view(8));

    // A shorter, quieter file replaces it mid-flight: the playhead must return
    // to zero, or the read would run past the end of the new buffer.
    player.play(makeFile(1, 4, 48000.0, 0.25F), 48000.0, 1.0F, 1);

    block.silence();
    player.render(block.view(8));

    CHECK(block.storage[0][0] == doctest::Approx(0.25));
    CHECK(block.storage[0][3] == doctest::Approx(0.25));
    CHECK(block.storage[0][4] == doctest::Approx(0.0));
    CHECK_FALSE(player.isPlaying());
}

TEST_CASE("a replaced buffer is held until the audio thread has provably left it")
{
    AuditionPlayer player;

    player.play(makeFile(1, 16, 48000.0), 48000.0, 1.0F, 10);
    CHECK(player.retainedCount() == 1);

    player.play(makeFile(1, 16, 48000.0), 48000.0, 1.0F, 10);
    CHECK(player.retainedCount() == 2);

    // The block the swap happened in may still be rendering.
    player.collect(10);
    CHECK(player.retainedCount() == 2);

    player.collect(10 + AuditionPlayer::retirementGraceBlocks);
    CHECK(player.retainedCount() == 1);

    // A stopped device has no callback to be inside anything.
    player.play(makeFile(1, 16, 48000.0), 48000.0, 1.0F, 12);
    CHECK(player.retainedCount() == 2);
    player.collect(12, true);
    CHECK(player.retainedCount() == 1);
}

TEST_CASE("rendering a preview allocates nothing")
{
    AuditionPlayer player;
    Block          block(2, 128);

    player.play(makeFile(2, 4800, 44100.0), 48000.0, 0.8F, 0);

    rt::resetViolations();

    {
        const rt::ScopedRealtimeContext scope;

        for (int index = 0; index < 8; ++index)
            player.render(block.view(128));
    }

    CHECK(rt::allocationViolations() == 0);
}

TEST_CASE("an empty or broken file is refused rather than played")
{
    AuditionPlayer player;
    Block          block(1, 16);

    player.play(nullptr, 48000.0, 1.0F, 0);
    CHECK_FALSE(player.isPlaying());

    auto empty          = std::make_shared<AudioFileData>();
    empty->sampleRate   = 48000.0;
    empty->channelCount = 2;
    empty->frameCount   = 0;

    player.play(empty, 48000.0, 1.0F, 0);
    CHECK_FALSE(player.isPlaying());

    player.render(block.view(16));
    CHECK(block.storage[0][0] == doctest::Approx(0.0));
}
