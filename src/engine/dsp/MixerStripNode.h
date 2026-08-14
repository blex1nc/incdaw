#pragma once

#include "engine/core/LevelMeter.h"
#include "engine/core/Smoother.h"
#include "engine/graph/Node.h"

#include <atomic>

namespace incdaw::engine::dsp {

/// One mixer strip: sums its inputs, then applies polarity, pan, volume and
/// mute, and measures what came out.
///
/// Deliberately one node rather than a chain of small ones. Splitting it would
/// cost a buffer and an indirection per stage per block for arithmetic that
/// fits in a single pass, and none of the stages is independently useful —
/// nobody wants a pan without a fader. Insert effects (Phase 13, Phase 15)
/// chain *in front of* a strip, not inside it, which is what keeps them
/// orderable and bypassable.
///
/// Pan is constant power: centre is -3 dB on both sides rather than unity, so
/// that sweeping a source across the image keeps its loudness (D-021). Channels
/// beyond the first two are passed through unpanned; a surround pan law is a
/// later decision, not an accident of this one.
class MixerStripNode final : public Node {
public:
    MixerStripNode() = default;

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    void setGain(Sample gain) noexcept;
    [[nodiscard]] Sample gain() const noexcept { return gain_.load(std::memory_order_relaxed); }

    /// -1 hard left, 0 centre, +1 hard right.
    void setPan(double pan) noexcept;
    [[nodiscard]] double pan() const noexcept { return pan_.load(std::memory_order_relaxed); }

    void setMuted(bool muted) noexcept;
    [[nodiscard]] bool isMuted() const noexcept { return muted_.load(std::memory_order_relaxed); }

    /// Flips the signal's sign. Used to fix a mis-wired source, so it must not
    /// change the level.
    void setPolarityInverted(bool inverted) noexcept;
    [[nodiscard]] bool isPolarityInverted() const noexcept
    {
        return polarityInverted_.load(std::memory_order_relaxed);
    }

    /// Metering, written on the audio thread and read by the UI.
    [[nodiscard]] const LevelMeter& meter() const noexcept { return meter_; }

    [[nodiscard]] const char* name() const noexcept override { return "MixerStrip"; }

    /// The pan law itself, exposed so that the UI and the tests use the same
    /// arithmetic the audio thread does.
    static void panGains(double pan, Sample& left, Sample& right) noexcept;

private:
    void refreshTargets() noexcept;

    std::atomic<Sample> gain_{Sample{1}};
    std::atomic<double> pan_{0.0};
    std::atomic<bool>   muted_{false};
    std::atomic<bool>   polarityInverted_{false};

    Smoother   left_;
    Smoother   right_;
    LevelMeter meter_;
};

} // namespace incdaw::engine::dsp
