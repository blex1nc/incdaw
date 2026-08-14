#include "project/MidiCapture.h"

#include <algorithm>
#include <cmath>

namespace incdaw::project {
namespace {

MidiEventType toEventType(engine::RecordedEvent::Kind kind) noexcept
{
    switch (kind) {
        case engine::RecordedEvent::Kind::note:          return MidiEventType::note;
        case engine::RecordedEvent::Kind::controlChange: return MidiEventType::controlChange;
        case engine::RecordedEvent::Kind::pitchBend:     return MidiEventType::pitchBend;
    }
    return MidiEventType::note;
}

/// splitmix64. Chosen because it is a handful of lines, has no state to carry,
/// and gives the same sequence on every platform — a humanise that differed
/// between machines would break the golden-file audio tests.
std::uint64_t nextRandom(std::uint64_t& state) noexcept
{
    state += 0x9E3779B97F4A7C15ull;
    std::uint64_t result = state;
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ull;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EBull;
    return result ^ (result >> 31);
}

} // namespace

void appendRecordedEvents(std::vector<MidiEvent>& destination,
                          const std::vector<engine::RecordedEvent>& events)
{
    destination.reserve(destination.size() + events.size());

    for (const engine::RecordedEvent& recorded : events) {
        MidiEvent event;
        event.type         = toEventType(recorded.kind);
        event.tick         = recorded.tick;
        event.duration     = recorded.duration;
        event.channel      = recorded.channel;
        event.key          = recorded.key;
        event.value        = recorded.value;
        event.releaseValue = recorded.releaseValue;

        destination.push_back(std::move(event));
    }

    std::stable_sort(destination.begin(), destination.end(),
                     [](const MidiEvent& a, const MidiEvent& b) { return a.tick < b.tick; });
}

void quantizeNoteStarts(std::vector<MidiEvent>& events, Tick grid, double strength)
{
    if (grid <= 0)
        return;

    strength = std::clamp(strength, 0.0, 1.0);

    for (MidiEvent& event : events) {
        if (event.type != MidiEventType::note)
            continue;

        // Round to nearest, with floor semantics that also work for the
        // negative ticks a count-in produces.
        const Tick remainder = ((event.tick % grid) + grid) % grid;
        const Tick target    = remainder * 2 >= grid ? event.tick - remainder + grid
                                                     : event.tick - remainder;

        const auto moved = static_cast<Tick>(std::llround(
            static_cast<double>(event.tick) + strength * static_cast<double>(target - event.tick)));

        event.tick = moved;
    }

    std::stable_sort(events.begin(), events.end(),
                     [](const MidiEvent& a, const MidiEvent& b) { return a.tick < b.tick; });
}

void humanizeNoteStarts(std::vector<MidiEvent>& events, Tick maxTicks, std::uint64_t seed)
{
    if (maxTicks <= 0)
        return;

    std::uint64_t state = seed;
    const auto    span  = static_cast<std::uint64_t>(maxTicks) * 2 + 1;

    for (MidiEvent& event : events) {
        if (event.type != MidiEventType::note)
            continue;

        const auto displacement = static_cast<Tick>(nextRandom(state) % span) - maxTicks;

        // Notes are never pushed before the pattern start: a negative tick would
        // place them outside the pattern and they would simply never play.
        event.tick = std::max<Tick>(0, event.tick + displacement);
    }

    std::stable_sort(events.begin(), events.end(),
                     [](const MidiEvent& a, const MidiEvent& b) { return a.tick < b.tick; });
}

} // namespace incdaw::project
