#pragma once

#include "engine/graph/Node.h"

#include <atomic>

namespace incdaw::engine::dsp {

/// Volume, pan, polarity, mute and metering for one strip.
///
/// Used for both a channel's own strip and a mixer node's, because the DSP is
/// identical and two copies of a pan law is two places for it to be wrong. The
/// two are still distinct in the model and in the graph: a channel strip sits
/// at the source and is saved with the channel, a mixer strip sits downstream
/// and is saved with the mixer node. This class does not know which it is.
///
/// Pan is constant-power: a centred signal and a hard-panned one have the same
/// perceived loudness, which linear panning does not give. Both gains are
/// smoothed for the same reason GainNode smooths — a stepped gain change is a
/// discontinuity, and a discontinuity is a click.
class ChannelStripNode final : public Node {
public:
    ChannelStripNode() = default;

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    /// Linear gain. Mute is kept separate from a zero volume so that unmuting
    /// restores the level the user set.
    void setVolume(Sample volume) noexcept { volume_.store(volume, std::memory_order_relaxed); }
    void setPan(Sample pan) noexcept { pan_.store(pan, std::memory_order_relaxed); }
    void setMuted(bool muted) noexcept { muted_.store(muted, std::memory_order_relaxed); }

    /// Inverts the signal. A polarity flip is not a 180° phase shift and the
    /// distinction matters: this is what fixes a mis-wired pair of mics, and it
    /// is applied before the fader so that metering shows what the strip
    /// actually contributes.
    void setPolarityFlipped(bool flipped) noexcept { polarity_.store(flipped, std::memory_order_relaxed); }
    [[nodiscard]] bool polarityFlipped() const noexcept { return polarity_.load(std::memory_order_relaxed); }

    [[nodiscard]] Sample volume() const noexcept { return volume_.load(std::memory_order_relaxed); }
    [[nodiscard]] Sample pan()    const noexcept { return pan_.load(std::memory_order_relaxed); }
    [[nodiscard]] bool   muted()  const noexcept { return muted_.load(std::memory_order_relaxed); }

    /// Peak absolute sample of the last block. Read by the UI for the meters;
    /// never read by the audio thread.
    [[nodiscard]] Sample lastPeak() const noexcept { return peak_.load(std::memory_order_relaxed); }

    /// Root mean square of the last block.
    ///
    /// Peak alone is a poor loudness display — it reacts to a single sample and
    /// says nothing about how loud a passage feels. RMS is the cheap, honest
    /// second number, and it is the quantity a LUFS meter integrates over time,
    /// so the loudness work in Phase 17 extends this rather than replacing it.
    [[nodiscard]] Sample lastRms() const noexcept { return rms_.load(std::memory_order_relaxed); }

    [[nodiscard]] const char* name() const noexcept override { return "Channel"; }

private:
    std::atomic<Sample> volume_{Sample{1}};
    std::atomic<Sample> pan_{Sample{0}};
    std::atomic<bool>   muted_{false};
    std::atomic<Sample> peak_{Sample{0}};
    std::atomic<Sample> rms_{Sample{0}};
    std::atomic<bool>   polarity_{false};

    Sample     currentLeft_  = Sample{1};
    Sample     currentRight_ = Sample{1};
    bool       primed_       = false;
    SampleRate sampleRate_   = 0.0;
};

} // namespace incdaw::engine::dsp
