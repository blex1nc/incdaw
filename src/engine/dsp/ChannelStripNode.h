#pragma once

#include "engine/graph/Node.h"

#include <atomic>

namespace incdaw::engine::dsp {

/// A channel's own volume, pan and mute.
///
/// This is NOT the mixer (Phase 10). A channel's level and position are
/// properties of the sound source itself — they exist before any mixer track is
/// assigned, they are what the channel rack shows, and they are saved with the
/// channel. The mixer, when it arrives, sits downstream of this and does not
/// replace it.
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

    [[nodiscard]] Sample volume() const noexcept { return volume_.load(std::memory_order_relaxed); }
    [[nodiscard]] Sample pan()    const noexcept { return pan_.load(std::memory_order_relaxed); }
    [[nodiscard]] bool   muted()  const noexcept { return muted_.load(std::memory_order_relaxed); }

    /// Peak absolute sample of the last block, per channel pair. Read by the
    /// UI for the channel rack meters; never read by the audio thread.
    [[nodiscard]] Sample lastPeak() const noexcept { return peak_.load(std::memory_order_relaxed); }

    [[nodiscard]] const char* name() const noexcept override { return "Channel"; }

private:
    std::atomic<Sample> volume_{Sample{1}};
    std::atomic<Sample> pan_{Sample{0}};
    std::atomic<bool>   muted_{false};
    std::atomic<Sample> peak_{Sample{0}};

    Sample     currentLeft_  = Sample{1};
    Sample     currentRight_ = Sample{1};
    bool       primed_       = false;
    SampleRate sampleRate_   = 0.0;
};

} // namespace incdaw::engine::dsp
