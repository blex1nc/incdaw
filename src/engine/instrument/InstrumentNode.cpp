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

    // A discontinuity means every sounding voice was started at a position the
    // transport has left. Without this, seeking or looping leaves notes hanging
    // until something happens to release them — the classic "stuck note after
    // loop" bug.
    if (expectedNextFrame_ >= 0 && context.playPosition != expectedNextFrame_)
        instrument_->allNotesOff();

    expectedNextFrame_ = context.playPosition + context.frameCount;

    if (sequenceEnabled_.load(std::memory_order_relaxed) && tempoMap_ != nullptr)
        sequence_.collectForRange(blockMidi_, context.playPosition, context.frameCount, *tempoMap_);

    // Live input is merged into the same buffer, so a played note and a
    // sequenced note on the same key reach the instrument identically.
    if (context.liveMidi != nullptr)
        for (const MidiMessage& message : *context.liveMidi)
            (void)blockMidi_.insert(message);

    instrument_->processBlock(context.output, blockMidi_);
}

} // namespace incdaw::engine
