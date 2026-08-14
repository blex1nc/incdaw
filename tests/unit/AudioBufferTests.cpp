#include "doctest.h"

#include "engine/core/AudioBufferPool.h"

#include <cmath>
#include <limits>

using namespace incdaw::engine;

TEST_CASE("a pool hands out independent, silent buffers")
{
    AudioBufferPool pool;
    pool.allocate(3, 2, 128);

    CHECK(pool.bufferCount() == 3);
    CHECK(pool.channelCount() == 2);
    CHECK(pool.frameCount() == 128);

    for (std::size_t index = 0; index < 3; ++index)
        CHECK(pool.buffer(index).peak() == doctest::Approx(0.0f));

    // Writing to one buffer must not disturb another: the pool is one
    // allocation, and an off-by-one in the channel pointers would show here.
    pool.buffer(1).channel(0)[64] = 1.0f;

    CHECK(pool.buffer(0).peak() == doctest::Approx(0.0f));
    CHECK(pool.buffer(1).peak() == doctest::Approx(1.0f));
    CHECK(pool.buffer(2).peak() == doctest::Approx(0.0f));
}

TEST_CASE("channels within a buffer are independent")
{
    AudioBufferPool pool;
    pool.allocate(1, 4, 32);

    const auto buffer = pool.buffer(0);
    buffer.channel(2)[10] = 0.5f;

    CHECK(buffer.channel(0)[10] == doctest::Approx(0.0f));
    CHECK(buffer.channel(1)[10] == doctest::Approx(0.0f));
    CHECK(buffer.channel(2)[10] == doctest::Approx(0.5f));
    CHECK(buffer.channel(3)[10] == doctest::Approx(0.0f));
}

TEST_CASE("an out-of-range buffer index yields an empty view rather than undefined behaviour")
{
    AudioBufferPool pool;
    pool.allocate(2, 2, 16);

    CHECK(pool.buffer(5).isEmpty());
    CHECK(pool.buffer(5).channelCount() == 0);
}

TEST_CASE("a degenerate allocation resets the pool instead of half-allocating")
{
    AudioBufferPool pool;
    pool.allocate(4, 2, 64);
    REQUIRE(pool.bufferCount() == 4);

    pool.allocate(0, 2, 64);
    CHECK(pool.bufferCount() == 0);
    CHECK(pool.buffer(0).isEmpty());
}

TEST_CASE("addFrom sums and copyFrom replaces")
{
    AudioBufferPool pool;
    pool.allocate(2, 2, 8);

    const auto destination = pool.buffer(0);
    const auto source      = pool.buffer(1);

    for (FrameCount frame = 0; frame < 8; ++frame) {
        destination.channel(0)[frame] = 0.25f;
        source.channel(0)[frame]      = 0.5f;
    }

    destination.addFrom(source);
    CHECK(destination.channel(0)[0] == doctest::Approx(0.75f));

    destination.copyFrom(source);
    CHECK(destination.channel(0)[0] == doctest::Approx(0.5f));
}

TEST_CASE("mismatched channel and frame counts process the overlap without reading out of bounds")
{
    // A routing mismatch is a fact of a flexible graph, not an error. What it
    // must never be is a buffer overrun.
    AudioBufferPool wide;
    AudioBufferPool narrow;
    wide.allocate(1, 4, 16);
    narrow.allocate(1, 2, 8);

    for (FrameCount frame = 0; frame < 8; ++frame) {
        narrow.buffer(0).channel(0)[frame] = 1.0f;
        narrow.buffer(0).channel(1)[frame] = 1.0f;
    }

    wide.buffer(0).addFrom(narrow.buffer(0));

    CHECK(wide.buffer(0).channel(0)[0] == doctest::Approx(1.0f));
    CHECK(wide.buffer(0).channel(1)[7] == doctest::Approx(1.0f));
    CHECK(wide.buffer(0).channel(0)[8] == doctest::Approx(0.0f));   // beyond source frames
    CHECK(wide.buffer(0).channel(2)[0] == doctest::Approx(0.0f));   // beyond source channels
    CHECK(wide.buffer(0).channel(3)[0] == doctest::Approx(0.0f));
}

TEST_CASE("a sub-block is a window into the same memory")
{
    AudioBufferPool pool;
    pool.allocate(1, 1, 100);

    const auto whole = pool.buffer(0);
    const auto part  = whole.subBlock(40, 20);

    CHECK(part.frameCount() == 20);

    part.channel(0)[0]  = 1.0f;
    part.channel(0)[19] = 2.0f;

    // Writing through the window must be visible in the parent at the offset.
    CHECK(whole.channel(0)[40] == doctest::Approx(1.0f));
    CHECK(whole.channel(0)[59] == doctest::Approx(2.0f));
    CHECK(whole.channel(0)[39] == doctest::Approx(0.0f));
    CHECK(whole.channel(0)[60] == doctest::Approx(0.0f));
}

TEST_CASE("sub-blocks nest, which is what block splitting relies on")
{
    AudioBufferPool pool;
    pool.allocate(1, 1, 64);

    const auto whole = pool.buffer(0);
    const auto outer = whole.subBlock(8, 32);
    const auto inner = outer.subBlock(4, 8);

    inner.channel(0)[0] = 3.0f;
    CHECK(whole.channel(0)[12] == doctest::Approx(3.0f));
}

TEST_CASE("clearing a sub-block leaves the rest of the buffer alone")
{
    AudioBufferPool pool;
    pool.allocate(1, 1, 32);

    const auto whole = pool.buffer(0);
    for (FrameCount frame = 0; frame < 32; ++frame)
        whole.channel(0)[frame] = 1.0f;

    whole.subBlock(8, 8).clear();

    CHECK(whole.channel(0)[7] == doctest::Approx(1.0f));
    CHECK(whole.channel(0)[8] == doctest::Approx(0.0f));
    CHECK(whole.channel(0)[15] == doctest::Approx(0.0f));
    CHECK(whole.channel(0)[16] == doctest::Approx(1.0f));
}

TEST_CASE("peak reports the largest magnitude, including negatives")
{
    AudioBufferPool pool;
    pool.allocate(1, 2, 4);

    const auto buffer = pool.buffer(0);
    buffer.channel(0)[0] = 0.3f;
    buffer.channel(1)[2] = -0.9f;

    CHECK(buffer.peak() == doctest::Approx(0.9f));
}

TEST_CASE("NaN and infinity are detected")
{
    // One misbehaving plugin can silence an entire mix bus with a single NaN,
    // and the symptom looks nothing like the cause.
    AudioBufferPool pool;
    pool.allocate(1, 1, 4);

    const auto buffer = pool.buffer(0);
    CHECK_FALSE(buffer.hasNonFiniteSamples());

    buffer.channel(0)[2] = std::numeric_limits<float>::quiet_NaN();
    CHECK(buffer.hasNonFiniteSamples());

    buffer.channel(0)[2] = std::numeric_limits<float>::infinity();
    CHECK(buffer.hasNonFiniteSamples());

    buffer.channel(0)[2] = -std::numeric_limits<float>::infinity();
    CHECK(buffer.hasNonFiniteSamples());

    buffer.channel(0)[2] = 0.0f;
    CHECK_FALSE(buffer.hasNonFiniteSamples());
}

TEST_CASE("applyGain scales every channel")
{
    AudioBufferPool pool;
    pool.allocate(1, 2, 4);

    const auto buffer = pool.buffer(0);
    for (std::size_t channel = 0; channel < 2; ++channel)
        for (FrameCount frame = 0; frame < 4; ++frame)
            buffer.channel(channel)[frame] = 1.0f;

    buffer.applyGain(0.5f);

    CHECK(buffer.channel(0)[0] == doctest::Approx(0.5f));
    CHECK(buffer.channel(1)[3] == doctest::Approx(0.5f));
}
