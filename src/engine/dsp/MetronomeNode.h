#pragma once

#include "engine/graph/Node.h"
#include "engine/transport/Transport.h"

#include <atomic>
#include <vector>

namespace incdaw::engine::dsp {

/// A click on every beat, placed on the exact frame the beat falls on.
///
/// Beyond being a metronome, this is the instrument the transport is *measured*
/// with: a click is a single unambiguous sample onset, so a test can assert the
/// exact frame it lands on. The Phase 3 exit criterion is expressed that way
/// (docs/ROADMAP.md).
///
/// Beat positions are resolved through the tempo map for each block, so tempo
/// changes and loop wraps are honoured without the node tracking any musical
/// state of its own — state that would have to be re-derived after every seek
/// anyway, and would drift if it were not.
class MetronomeNode final : public Node {
public:
    explicit MetronomeNode(const Transport& transport) noexcept : transport_(&transport) {}

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    void setEnabled(bool enabled) noexcept { enabled_.store(enabled, std::memory_order_relaxed); }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_.load(std::memory_order_relaxed); }

    void setAmplitude(Sample amplitude) noexcept { amplitude_.store(amplitude, std::memory_order_relaxed); }

    /// Frame offsets, within the last processed block, at which a click began.
    /// Recorded for tests and diagnostics; writing to it is realtime-safe
    /// because the storage is sized in `prepare`.
    [[nodiscard]] const std::vector<FrameCount>& lastBlockClickOffsets() const noexcept { return clickOffsets_; }
    [[nodiscard]] std::size_t lastBlockClickCount() const noexcept { return clickCount_; }

    [[nodiscard]] const char* name() const noexcept override { return "Metronome"; }

private:
    void triggerClick(bool isDownbeat) noexcept;

    const Transport*    transport_ = nullptr;
    std::atomic<bool>   enabled_{true};
    std::atomic<Sample> amplitude_{0.4f};

    SampleRate sampleRate_ = 48000.0;

    // Click envelope state, carried across blocks so a click that starts near
    // the end of one block finishes in the next.
    FrameCount clickRemaining_ = 0;
    double     clickPhase_     = 0.0;
    double     clickIncrement_ = 0.0;
    FrameCount clickLength_    = 0;

    std::vector<FrameCount> clickOffsets_;
    std::size_t             clickCount_ = 0;
};

} // namespace incdaw::engine::dsp
