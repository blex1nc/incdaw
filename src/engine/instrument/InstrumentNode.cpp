#include "engine/instrument/InstrumentNode.h"

namespace incdaw::engine {

void InstrumentNode::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    if (instrument_ != nullptr)
        instrument_->prepare(sampleRate, maxBlockSize);

    expectedNextFrame_ = -1;
    blockMidi_.clear();
}

void InstrumentNode::process(const ProcessContext& context) noexcept
{
    if (instrument_ == nullptr || context.frameCount <= 0)
        return;

    blockMidi_.clear();

    if (context.playing) {
        // A discontinuity means every sounding voice was started at a position
        // the transport has left. Without this, seeking or looping leaves notes
        // hanging until something happens to release them — the classic "stuck
        // note after loop" bug.
        if (expectedNextFrame_ >= 0 && context.playPosition != expectedNextFrame_)
            instrument_->allNotesOff();

        expectedNextFrame_ = context.playPosition + context.frameCount;

        if (sequenceEnabled_.load(std::memory_order_relaxed) && tempoMap_ != nullptr)
            sequence_.collectForRange(blockMidi_, context.playPosition, context.frameCount,
                                      *tempoMap_);
    } else if (expectedNextFrame_ >= 0) {
        // The first block after the transport stopped: the sequence's notes
        // were started at a position nothing will leave now, so they are ended
        // here, once. Every later stopped block leaves the instrument alone —
        // it is what a live keyboard is playing into, and killing its voices
        // every block is how a held note becomes a buzz.
        instrument_->allNotesOff();
        expectedNextFrame_ = -1;
    }

    // Live input is merged into the same buffer, so a played note and a
    // sequenced note on the same key reach the instrument identically.
    if (context.liveMidi != nullptr)
        for (const MidiMessage& message : *context.liveMidi)
            (void)blockMidi_.insert(message);

    instrument_->processBlock(context.output, blockMidi_);
}

} // namespace incdaw::engine
