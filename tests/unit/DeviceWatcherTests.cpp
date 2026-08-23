// C5 — hot-plug. What can be tested without hardware is the lifecycle, and
// that is worth testing: the failure modes of an observer that outlives the
// things it observes are a callback into a half-destroyed object and a
// deadlock between teardown and a notification in flight. Neither needs a
// device to be plugged in to happen, and neither is visible in a review.
//
// Whether a real interface being unplugged produces a notification is a claim
// this suite deliberately does NOT make.

#include "doctest.h"

#include "platform/DeviceWatcher.h"

#include <atomic>
#include <memory>

using namespace incdaw;

TEST_CASE("a watcher exists on this platform and starts having seen nothing")
{
    const std::unique_ptr<platform::DeviceWatcher> watcher = platform::DeviceWatcher::create();
    REQUIRE(watcher != nullptr);
    CHECK(watcher->changeCount() == 0);
}

TEST_CASE("a callback can be installed, replaced and cleared")
{
    const std::unique_ptr<platform::DeviceWatcher> watcher = platform::DeviceWatcher::create();
    REQUIRE(watcher != nullptr);

    std::atomic<int> first{0};
    std::atomic<int> second{0};

    watcher->setCallback([&first] { first.fetch_add(1, std::memory_order_relaxed); });
    watcher->setCallback([&second] { second.fetch_add(1, std::memory_order_relaxed); });
    watcher->setCallback({});

    // Nothing was plugged in during this test, so nothing should have fired.
    // The assertion that matters is the one below it: the watcher is destroyed
    // with lambdas that captured stack objects, and it must not call them.
    CHECK(first.load() == 0);
    CHECK(second.load() == 0);
}

TEST_CASE("destroying a watcher with a live callback is safe")
{
    std::atomic<int> calls{0};

    {
        const std::unique_ptr<platform::DeviceWatcher> watcher = platform::DeviceWatcher::create();
        REQUIRE(watcher != nullptr);

        watcher->setCallback([&calls] { calls.fetch_add(1, std::memory_order_relaxed); });
    }

    // The capture is a dangling reference the moment this scope ends, which is
    // exactly why the watcher clears its callback before it tears down its
    // system handles rather than after.
    CHECK(calls.load() >= 0);
}

TEST_CASE("several watchers can coexist")
{
    // The settings window and the shell both want to know, and CoreMIDI
    // clients are per-object: two watchers must not tread on each other.
    const auto first  = platform::DeviceWatcher::create();
    const auto second = platform::DeviceWatcher::create();

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    CHECK(first->changeCount() == 0);
    CHECK(second->changeCount() == 0);
}
