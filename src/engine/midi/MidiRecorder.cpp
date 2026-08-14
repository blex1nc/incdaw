#include "engine/midi/MidiRecorder.h"

#include "engine/midi/MidiMessage.h"

#include <algorithm>
#include <array>

namespace incdaw::engine {

void MidiRecorder::capture(const MidiBuffer& buffer, FramePosition blockStartFrame) noexcept
{
    for (const MidiMessage& message : buffer) {
        CapturedMessage captured;
        captured.frame  = blockStartFrame + message.frameOffset;
        captured.status = message.status;
        captured.data1  = message.data1;
        captured.data2  = message.data2;

        if (queue_.push(captured))
            captured_.fetch_add(1, std::memory_order_relaxed);
        else
            dropped_.fetch_add(1, std::memory_order_relaxed);
    }
}

void MidiRecorder::drainInto(std::vector<RecordedEvent>& events, const TempoMap& tempoMap,
                             FramePosition endFrame)
{
    // Index of the event opened by a still-sounding note, per channel
    // and note number. -1 means "not sounding".
    //
    // A flat array rather than a map: 16 x 128 is small, and it keeps the
    // lookup free of allocation should this ever move closer to the audio path.
    std::array<int, 16 * 128> sounding{};
    sounding.fill(-1);

    const auto slot = [](int channel, int note) {
        return (channel & 0x0F) * 128 + (note & 0x7F);
    };

    CapturedMessage captured;
    while (queue_.pop(captured)) {
        const MidiMessage message{0, captured.status, captured.data1, captured.data2};
        const Tick        tick = tempoMap.tickForFrame(captured.frame);

        if (message.isNoteOn()) {
            const int index = slot(message.channel(), message.noteNumber());

            // A second note-on without an intervening note-off happens with
            // retriggering keyboards. Close the first rather than leaking it.
            if (sounding[static_cast<std::size_t>(index)] >= 0) {
                auto& open = events[static_cast<std::size_t>(sounding[static_cast<std::size_t>(index)])];
                open.duration = std::max<Tick>(1, tick - open.tick);
            }

            RecordedEvent event;
            event.kind     = RecordedEvent::Kind::note;
            event.tick     = tick;
            event.duration = 0;
            event.channel  = message.channel();
            event.key      = message.noteNumber();
            event.value    = message.velocity();

            events.push_back(event);
            sounding[static_cast<std::size_t>(index)] = static_cast<int>(events.size()) - 1;
            continue;
        }

        if (message.isNoteOff()) {
            const int index    = slot(message.channel(), message.noteNumber());
            const int eventIndex = sounding[static_cast<std::size_t>(index)];

            if (eventIndex >= 0) {
                auto& open = events[static_cast<std::size_t>(eventIndex)];
                // At least one tick: a zero-length note is invisible in the
                // editor and silent on playback, which reads as a dropped note.
                open.duration      = std::max<Tick>(1, tick - open.tick);
                open.releaseValue  = message.velocity();
                sounding[static_cast<std::size_t>(index)] = -1;
            }
            continue;
        }

        RecordedEvent event;
        event.tick    = tick;
        event.channel = message.channel();

        if (message.isControlChange()) {
            event.kind  = RecordedEvent::Kind::controlChange;
            event.key   = message.data1;
            event.value = message.data2;
        } else if (message.isPitchBend()) {
            event.kind  = RecordedEvent::Kind::pitchBend;
            event.key   = 0;
            event.value = message.pitchBendValue();
        } else {
            continue;   // system and unhandled messages are not pattern content
        }

        events.push_back(event);
    }

    // Close anything still held. The player lifting their hand after recording
    // stops must not cost them the note.
    const Tick endTick = tempoMap.tickForFrame(endFrame);
    for (const int eventIndex : sounding) {
        if (eventIndex < 0)
            continue;

        auto& open = events[static_cast<std::size_t>(eventIndex)];
        if (open.duration == 0)
            open.duration = std::max<Tick>(1, endTick - open.tick);
    }

    std::stable_sort(events.begin(), events.end(),
                     [](const RecordedEvent& a, const RecordedEvent& b) { return a.tick < b.tick; });
}

void MidiRecorder::reset() noexcept
{
    CapturedMessage discard;
    while (queue_.pop(discard)) { }

    dropped_.store(0, std::memory_order_relaxed);
    captured_.store(0, std::memory_order_relaxed);
}

} // namespace incdaw::engine
