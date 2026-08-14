#include "engine/instrument/Instrument.h"

namespace incdaw::engine {

void Instrument::processBlock(const AudioBufferView& output, const MidiBuffer& midi) noexcept
{
    const FrameCount total = output.frameCount();
    if (total <= 0)
        return;

    FrameCount rendered = 0;
    std::size_t next    = 0;

    // Walk the block, stopping at each message. The buffer is already sorted by
    // frame offset, so this is a single forward pass.
    while (rendered < total) {
        // Apply every message due at this exact frame before rendering on.
        while (next < midi.size() && midi[next].frameOffset <= rendered) {
            handleMessage(midi[next]);
            ++next;
        }

        // Render up to the next message, or to the end of the block.
        FrameCount until = total;
        if (next < midi.size()) {
            const FrameCount offset = midi[next].frameOffset;
            if (offset > rendered && offset < total)
                until = offset;
        }

        const FrameCount span = until - rendered;
        if (span <= 0)
            break;   // defensive: a malformed buffer must not spin here

        renderRange(output.subBlock(rendered, span), span);
        rendered += span;
    }

    // Messages timestamped past the end of the block still belong to it — they
    // were placed there by the input layer and dropping them would lose notes.
    while (next < midi.size()) {
        handleMessage(midi[next]);
        ++next;
    }
}

} // namespace incdaw::engine
