#include "engine/midi/MidiFeedback.h"

#include "engine/midi/MidiOutput.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine {

void MidiFeedback::setBindings(std::vector<Binding> bindings)
{
    if (bindings.size() > maxBindings)
        bindings.resize(maxBindings);

    const std::lock_guard<std::mutex> guard(mutex_);

    bindings_ = std::move(bindings);

    // -1 is "never sent". Every slot starts there so the surface is written in
    // full on the next flush rather than being left holding whatever the
    // previous project's mapping put there.
    lastSent_.assign(bindings_.size(), -1);

    for (std::size_t slot = 0; slot < maxBindings; ++slot)
        suppressed_[slot].store(-1, std::memory_order_relaxed);
}

std::size_t MidiFeedback::bindingCount() const
{
    const std::lock_guard<std::mutex> guard(mutex_);
    return bindings_.size();
}

int MidiFeedback::lastSentAt(std::size_t slot) const
{
    const std::lock_guard<std::mutex> guard(mutex_);
    return slot < lastSent_.size() ? lastSent_[slot] : -1;
}

std::size_t MidiFeedback::flush(MidiOutput& output)
{
    if (!enabled_.load(std::memory_order_acquire))
        return 0;

    const std::lock_guard<std::mutex> guard(mutex_);

    std::size_t sent = 0;

    for (std::size_t slot = 0; slot < bindings_.size(); ++slot) {
        const Binding& binding = bindings_[slot];
        if (!binding.active)
            continue;

        // What arrived from the hardware is adopted, never echoed. Answering a
        // fader with its own position is how a motorised one ends up hunting.
        const std::int32_t fromHardware = suppressed_[slot].exchange(-1, std::memory_order_relaxed);
        if (fromHardware >= 0) {
            lastSent_[slot] = fromHardware;
            continue;
        }

        const float span = binding.maxValue - binding.minValue;
        if (std::abs(span) < 1.0e-9f)
            continue;   // a range with no width has no position to report

        // The forward mapping is min + n * (max - min); this is that, solved
        // for n. An inverted mapping therefore reads back inverted, which is
        // the whole point of running the same binding backwards.
        const float value      = values_[slot].load(std::memory_order_relaxed);
        const float normalised = std::clamp((value - binding.minValue) / span, 0.0f, 1.0f);

        const int sevenBit = std::clamp(static_cast<int>(normalised * 127.0f + 0.5f), 0, 127);

        if (sevenBit == lastSent_[slot])
            continue;

        lastSent_[slot] = sevenBit;

        platform::TimestampedMidiMessage message;
        message.hostTimeNanos = 0;   // as soon as possible: this is a display, not a performance
        message.status = static_cast<std::uint8_t>(0xB0 | (binding.midiChannel & 0x0F));
        message.data1  = static_cast<std::uint8_t>(binding.controller & 0x7F);
        message.data2  = static_cast<std::uint8_t>(sevenBit);

        if (output.send(message))
            ++sent;
    }

    return sent;
}

} // namespace incdaw::engine
