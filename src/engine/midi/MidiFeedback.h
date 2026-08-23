#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace incdaw::engine {

class MidiOutput;

/// The return path of the mapping system.
///
/// A mapping is one-way until something sends the parameter's value back: a
/// motorised fader assigned to a channel's volume sits wherever it was last
/// pushed while automation moves the volume underneath it, and the two disagree
/// until somebody touches the fader. This is the same map read backwards — the
/// binding that says "CC 7 on channel 1 writes this parameter" also says "this
/// parameter reads back as CC 7 on channel 1".
///
/// **How a value gets here.** Every applier the compiler resolves for a mapped
/// parameter is wrapped in a tap, so anything that writes the parameter — an
/// automation lane, the hardware itself, a panel control going through the same
/// registry — records the value in the slot the mapping owns. The mirror is
/// therefore correct regardless of who wrote last, which is the property a
/// feedback path built around one particular writer never has.
///
/// **Why it does not feed back on itself.** A value that arrived FROM the
/// hardware is recorded as already sent. Without that, moving a fader produces
/// a CC back to the fader, which the fader answers, and a motorised one will
/// hunt around the position forever. Suppression is recorded where the
/// knowledge is — in the node that applied the incoming message — rather than
/// guessed at from timing.
///
/// **Threads.** `observe` and `suppress` are called from the audio thread and
/// are single atomic stores. `flush` runs on the shell's refresh, which is also
/// the resolution the hardware gets: a 7-bit CC has 128 positions, and sending
/// one per audio block would be thousands of messages a second saying the same
/// thing.
class MidiFeedback {
public:
    /// One slot per mapping. Well past any sensible control surface, and fixed
    /// so the audio thread's write is an array index rather than a lookup.
    static constexpr std::size_t maxBindings = 128;

    /// A mapping, read backwards.
    struct Binding {
        /// The channel feedback is sent on. A mapping that matches any channel
        /// has no channel to answer on, so it answers on 1 (zero here).
        int   midiChannel = 0;
        int   controller  = 0;

        /// The mapped range, as the forward direction uses it. Feedback
        /// inverts it, so an inverted mapping reads back inverted too.
        float minValue = 0.0f;
        float maxValue = 1.0f;

        bool  active = false;
    };

    /// Replaces the table. Called on the thread that compiles a graph.
    ///
    /// Every slot is marked unknown, so the first flush afterwards sends the
    /// whole surface rather than only what changed — which is what a control
    /// surface needs after a project loads, and what makes a mapping edit
    /// resolve itself rather than leaving one stale position behind.
    void setBindings(std::vector<Binding> bindings);

    [[nodiscard]] std::size_t bindingCount() const;

    void setEnabled(bool enabled) noexcept { enabled_.store(enabled, std::memory_order_release); }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_.load(std::memory_order_acquire); }

    /// Audio thread. Records the value a mapped parameter was just given.
    void observe(std::size_t slot, float value) noexcept
    {
        if (slot < maxBindings)
            values_[slot].store(value, std::memory_order_relaxed);
    }

    /// Audio thread. Records the 7-bit value that arrived from the hardware,
    /// so the next flush adopts it instead of sending it straight back.
    void suppress(std::size_t slot, int sevenBitValue) noexcept
    {
        if (slot < maxBindings)
            suppressed_[slot].store(sevenBitValue, std::memory_order_relaxed);
    }

    /// Sends a CC for every slot whose 7-bit value has moved. Returns how many
    /// were sent. Not for the audio thread.
    std::size_t flush(MidiOutput& output);

    /// The value currently mirrored for a slot. Diagnostics and tests.
    [[nodiscard]] float valueAt(std::size_t slot) const noexcept
    {
        return slot < maxBindings ? values_[slot].load(std::memory_order_relaxed) : 0.0f;
    }

    /// The last 7-bit value sent for a slot, or -1 if none has been.
    [[nodiscard]] int lastSentAt(std::size_t slot) const;

private:
    mutable std::mutex   mutex_;          ///< guards bindings_ and lastSent_; never taken by the audio thread
    std::vector<Binding> bindings_;
    std::vector<int>     lastSent_;

    std::array<std::atomic<float>, maxBindings>        values_{};
    std::array<std::atomic<std::int32_t>, maxBindings> suppressed_{};

    std::atomic<bool> enabled_{true};
};

} // namespace incdaw::engine
