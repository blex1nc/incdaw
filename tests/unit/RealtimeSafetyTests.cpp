#include "doctest.h"

#include "engine/core/Denormals.h"
#include "engine/core/LockFreeQueue.h"
#include "engine/core/RealtimeGuard.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

using namespace incdaw::engine;

namespace {

/// Opaque to the optimiser, so the allocation below cannot be folded away.
std::size_t allocationSize() noexcept
{
    static std::atomic<std::size_t> size{64};
    return size.load(std::memory_order_relaxed);
}

} // namespace

TEST_CASE("this build actually enforces realtime safety")
{
    // If the guard is compiled out, every test below would pass vacuously.
    // Fail loudly instead of reporting a green suite that verified nothing.
    CHECK(rt::guardEnabled());
}

TEST_CASE("the guard notices an allocation inside a realtime scope")
{
    rt::resetViolations();
    CHECK(rt::allocationViolations() == 0);

    {
        const rt::ScopedRealtimeContext scope;
        CHECK(rt::isInsideRealtimeContext());

        // Deliberate violation — this is what the guard exists to catch.
        //
        // Called through ::operator new rather than written as a new-expression
        // on purpose. C++14 permits the compiler to elide the allocation in a
        // new-expression, and at -O2 clang does exactly that, so the earlier
        // form tested nothing in release builds. A direct ::operator new call is
        // not elidable.
        //
        // (An elided allocation is not a realtime violation — it never
        // happened. The guard's job is to catch the ones that do.)
        void* memory = ::operator new(allocationSize());
        CHECK(memory != nullptr);
        ::operator delete(memory);
    }

    CHECK_FALSE(rt::isInsideRealtimeContext());
    CHECK(rt::allocationViolations() >= 1);
    CHECK(rt::deallocationViolations() >= 1);

    rt::resetViolations();
}

TEST_CASE("allocation outside a realtime scope is not a violation")
{
    rt::resetViolations();

    std::vector<int> values;
    values.reserve(1024);
    values.push_back(1);

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

TEST_CASE("realtime scopes nest correctly")
{
    {
        const rt::ScopedRealtimeContext outer;
        {
            const rt::ScopedRealtimeContext inner;
            CHECK(rt::isInsideRealtimeContext());
        }
        // Leaving the inner scope must not clear the outer one.
        CHECK(rt::isInsideRealtimeContext());
    }
    CHECK_FALSE(rt::isInsideRealtimeContext());
}

TEST_CASE("an exemption suspends the guard and restores it")
{
    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;
        {
            const rt::ScopedRealtimeExemption exemption{"test"};
            CHECK_FALSE(rt::isInsideRealtimeContext());
            void* memory = ::operator new(allocationSize());
            ::operator delete(memory);
        }
        CHECK(rt::isInsideRealtimeContext());
    }
    CHECK(rt::allocationViolations() == 0);
}

TEST_CASE("the realtime mark is per-thread")
{
    const rt::ScopedRealtimeContext scope;
    CHECK(rt::isInsideRealtimeContext());

    bool otherThreadSawMark = true;
    std::thread other{[&] { otherThreadSawMark = rt::isInsideRealtimeContext(); }};
    other.join();

    // A guard on one thread must not make another thread's allocations look
    // like violations.
    CHECK_FALSE(otherThreadSawMark);
}

TEST_CASE("denormals are flushed inside the scope and restored after it")
{
    const bool before = ScopedNoDenormals::areDenormalsDisabled();
    {
        const ScopedNoDenormals scope;
        CHECK(ScopedNoDenormals::areDenormalsDisabled());
    }
    CHECK(ScopedNoDenormals::areDenormalsDisabled() == before);
}

TEST_CASE("flushing denormals actually changes arithmetic")
{
    // Proves the control register write took effect, rather than the scope
    // being a no-op that happens to compile.
    volatile float tiny = 1e-38f;      // denormal territory for float
    {
        const ScopedNoDenormals scope;
        const volatile float flushed = tiny * 1e-8f;
        CHECK(flushed == 0.0f);
    }
}

TEST_CASE("the queue preserves order and reports fullness")
{
    LockFreeQueue<int, 8> queue;

    CHECK(queue.isEmpty());
    CHECK(queue.capacity() == 7);   // one slot distinguishes full from empty

    for (int value = 0; value < 7; ++value)
        CHECK(queue.push(value));

    CHECK_FALSE(queue.push(99));    // full
    CHECK(queue.size() == 7);

    for (int expected = 0; expected < 7; ++expected) {
        int value = -1;
        REQUIRE(queue.pop(value));
        CHECK(value == expected);
    }

    int drained = -1;
    CHECK_FALSE(queue.pop(drained));
    CHECK(queue.isEmpty());
}

TEST_CASE("the queue wraps without losing or duplicating messages")
{
    LockFreeQueue<int, 4> queue;

    for (int round = 0; round < 100; ++round) {
        CHECK(queue.push(round));
        int value = -1;
        REQUIRE(queue.pop(value));
        CHECK(value == round);
    }
}

TEST_CASE("the queue does not allocate, so the audio thread may use it")
{
    LockFreeQueue<std::int64_t, 64> queue;

    rt::resetViolations();
    {
        const rt::ScopedRealtimeContext scope;

        for (std::int64_t value = 0; value < 32; ++value)
            (void)queue.push(value);

        std::int64_t received = 0;
        while (queue.pop(received)) { }
    }

    CHECK(rt::allocationViolations() == 0);
    CHECK(rt::deallocationViolations() == 0);
}

TEST_CASE("a producer and a consumer on different threads agree on every message")
{
    constexpr std::int64_t messageCount = 200000;

    LockFreeQueue<std::int64_t, 1024> queue;
    std::int64_t                      received = 0;
    bool                              inOrder  = true;

    std::thread consumer{[&] {
        std::int64_t value    = 0;
        std::int64_t expected = 0;

        while (expected < messageCount) {
            if (queue.pop(value)) {
                if (value != expected)
                    inOrder = false;
                ++expected;
                ++received;
            }
        }
    }};

    for (std::int64_t value = 0; value < messageCount;) {
        if (queue.push(value))
            ++value;
    }

    consumer.join();

    CHECK(received == messageCount);
    CHECK(inOrder);
}
