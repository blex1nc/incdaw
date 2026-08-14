#pragma once

#include "engine/graph/Node.h"
#include "engine/instrument/Instrument.h"
#include "engine/midi/NoteSequence.h"
#include "engine/transport/TempoMap.h"

#include <atomic>
#include <memory>

namespace incdaw::engine {

/// A channel's sound source in the render graph.
///
/// Combines two MIDI streams into one instrument: the sequenced notes of the
/// pattern being played, and whatever the player is doing on a keyboard right
/// now. Both must be sample-accurate and both must reach the same instrument,
/// or a played note and a sequenced note on the same key would behave
/// differently.
class InstrumentNode final : public Node {
public:
    InstrumentNode(std::unique_ptr<Instrument> instrument, const TempoMap& tempoMap) noexcept
        : instrument_(std::move(instrument)), tempoMap_(&tempoMap) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override
    {
        return instrument_ != nullptr ? instrument_->name() : "Instrument";
    }

    [[nodiscard]] Instrument*       instrument()       noexcept { return instrument_.get(); }
    [[nodiscard]] const Instrument* instrument() const noexcept { return instrument_.get(); }

    /// The notes this node plays. Replaced off the audio thread when the
    /// pattern is edited; the graph is rebuilt around it.
    [[nodiscard]] NoteSequence&       sequence()       noexcept { return sequence_; }
    [[nodiscard]] const NoteSequence& sequence() const noexcept { return sequence_; }

    void setSequencePlaybackEnabled(bool enabled) noexcept
    {
        sequenceEnabled_.store(enabled, std::memory_order_relaxed);
    }

private:
    std::unique_ptr<Instrument> instrument_;
    const TempoMap*             tempoMap_ = nullptr;
    NoteSequence                sequence_;

    MidiBuffer                  blockMidi_;
    std::atomic<bool>           sequenceEnabled_{true};

    /// Where playback was expected to continue from. A mismatch means the
    /// transport jumped — a seek, a loop wrap, or a stop — and every sounding
    /// voice belongs to a position that is no longer current.
    FramePosition expectedNextFrame_ = -1;
};

} // namespace incdaw::engine
