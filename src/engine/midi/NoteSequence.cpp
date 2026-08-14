#include "engine/midi/NoteSequence.h"

#include <algorithm>

namespace incdaw::engine {

void NoteSequence::setNotes(std::vector<SequencedNote> notes)
{
    notes.erase(std::remove_if(notes.begin(), notes.end(),
                               [](const SequencedNote& note) {
                                   // Notes before the start of time, or with no
                                   // length, cannot be played and would only
                                   // produce a note-off with no note-on.
                                   return note.startTick < 0 || note.lengthTicks <= 0
                                       || note.velocity <= 0;
                               }),
                notes.end());

    std::sort(notes.begin(), notes.end(),
              [](const SequencedNote& a, const SequencedNote& b) { return a.startTick < b.startTick; });

    notes_ = std::move(notes);
    rebuildIndices();
}

void NoteSequence::clear() noexcept
{
    notes_.clear();
    byEnd_.clear();
    length_ = loopLength_;
}

void NoteSequence::setLoopLength(Tick ticks) noexcept
{
    loopLength_ = ticks > 0 ? ticks : 0;
    length_ = std::max(length_, loopLength_);
}

void NoteSequence::rebuildIndices()
{
    byEnd_.resize(notes_.size());
    for (std::uint32_t index = 0; index < notes_.size(); ++index)
        byEnd_[index] = index;

    std::sort(byEnd_.begin(), byEnd_.end(),
              [this](std::uint32_t a, std::uint32_t b) {
                  return notes_[a].endTick() < notes_[b].endTick();
              });

    Tick end = 0;
    for (const SequencedNote& note : notes_)
        end = std::max(end, note.endTick());

    length_ = std::max(end, loopLength_);
}

void NoteSequence::collectForRange(MidiBuffer& destination, FramePosition blockStartFrame,
                                   FrameCount frameCount, const TempoMap& tempoMap) const noexcept
{
    if (notes_.empty() || frameCount <= 0)
        return;

    const Tick fromTick = tempoMap.tickForFrame(blockStartFrame);
    const Tick toTick   = tempoMap.tickForFrame(blockStartFrame + frameCount);

    if (toTick < fromTick)
        return;

    const auto offsetFor = [&](Tick tick) noexcept {
        FrameCount offset = tempoMap.frameForTick(tick) - blockStartFrame;

        // Clamped rather than dropped. Tick resolution is coarser than frame
        // resolution at high tempi, so a note whose tick rounds just outside
        // the block would otherwise be lost entirely.
        if (offset < 0)
            offset = 0;
        if (offset >= frameCount)
            offset = frameCount - 1;

        return offset;
    };

    // ── Note-ons: notes starting in [fromTick, toTick) ──────────────────────
    {
        const auto first = std::lower_bound(notes_.begin(), notes_.end(), fromTick,
                                            [](const SequencedNote& note, Tick tick) {
                                                return note.startTick < tick;
                                            });

        for (auto note = first; note != notes_.end() && note->startTick < toTick; ++note)
            (void)destination.insert(
                MidiMessage::noteOn(note->channel, note->key, note->velocity, offsetFor(note->startTick)));
    }

    // ── Note-offs: notes ending in [fromTick, toTick) ───────────────────────
    {
        const auto first = std::lower_bound(byEnd_.begin(), byEnd_.end(), fromTick,
                                            [this](std::uint32_t index, Tick tick) {
                                                return notes_[index].endTick() < tick;
                                            });

        for (auto index = first; index != byEnd_.end(); ++index) {
            const SequencedNote& note = notes_[*index];
            if (note.endTick() >= toTick)
                break;

            (void)destination.insert(MidiMessage::noteOff(note.channel, note.key, 64,
                                                          offsetFor(note.endTick())));
        }
    }
}

} // namespace incdaw::engine
