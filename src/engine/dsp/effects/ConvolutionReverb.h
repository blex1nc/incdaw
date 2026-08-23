#pragma once

// Convolution reverb (A11) — uniform partitioned convolution.
//
// The naive way to convolve is O(N·M) and unusable: a two-second impulse at
// 48 kHz is ninety-six thousand taps per sample. The way it is actually done
// is to cut the impulse into equal partitions, transform each once, and
// multiply spectra — the same arithmetic, at a few per cent of the cost.
//
// Uniform partitions rather than non-uniform ones, deliberately. Non-uniform
// partitioning buys back the one block of latency this design costs, at the
// price of several schedules of different-sized FFTs running at different
// rates. That is the right trade for a mixing desk with no delay
// compensation; INCDAW has PDC, so the latency is reported and compensated
// and the simpler engine is the honest choice.
//
// INCDAW ships NO impulse responses (§20/§43). An impulse comes from the
// user's own file, carried in the insert's state as a named string; a
// convolver with no file generates a plausible hall from code so that adding
// one to a chain does something before a file has been found.

#include "engine/dsp/Fft.h"
#include "engine/dsp/effects/BuiltinEffect.h"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::engine::dsp {

class ConvolutionReverbEffect final : public BuiltinEffect {
public:
    /// Samples per partition. 256 is one block at every buffer size INCDAW
    /// offers, so the engine never straddles two host blocks per partition.
    static constexpr std::size_t partitionSize = 256;

    /// Transform length: twice the partition, because a linear convolution of
    /// two P-length signals is 2P long and a shorter transform would wrap it.
    static constexpr std::size_t transformSize = partitionSize * 2;

    /// Bins that are not redundant. The rest are the conjugate of these.
    static constexpr std::size_t binCount = transformSize / 2 + 1;

    /// The longest impulse the engine will hold. Two seconds at 48 kHz is
    /// 375 partitions — about as much convolution as one insert can cost
    /// before it should be a send, and it bounds the memory the two swap
    /// sets need.
    static constexpr double maxImpulseSeconds = 2.0;

    static constexpr std::size_t maxChannels = 2;

    /// Ids are frozen: they key the state blob and every saved preset.
    enum Param : std::uint32_t {
        mix          = 0,
        preDelayMs   = 1,
        wetGainDb    = 2,
        decaySeconds = 3,   ///< shortens the tail; 8 s leaves the impulse alone
        dampingHz    = 4,   ///< lowpass on the wet; 20 kHz is off
        lowCutHz     = 5,   ///< highpass on the wet; 20 Hz is off
        width        = 6,
        reverse      = 7,
    };

    /// The state key an impulse's path travels under.
    static constexpr const char* impulseKey = "ir";

    ConvolutionReverbEffect();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Convolution Reverb"; }

    /// One partition of latency, reported so delay compensation removes it.
    [[nodiscard]] FrameCount latencyFrames() const noexcept override
    {
        return static_cast<FrameCount>(partitionSize);
    }

    // ── The impulse ──────────────────────────────────────────────────────
    //
    // All of this is MAIN THREAD. Loading a file allocates, reads from disk
    // and runs several hundred transforms; none of that may happen where
    // audio is rendered.

    /// Loads `path`, resampling it to the session rate if it disagrees.
    /// Returns false and keeps the previous impulse if the file cannot be
    /// read — a missing impulse must not silence a mix.
    [[nodiscard]] bool loadImpulse(const std::filesystem::path& path);

    /// Replaces the impulse with one generated from code. What a convolver
    /// with no file plays, so that adding one to a chain does something.
    void generateDefaultImpulse();

    /// The path currently loaded, or empty for the generated impulse.
    [[nodiscard]] const std::string& impulsePath() const { return impulsePath_; }

    /// Partitions the live impulse occupies. Zero means silence.
    [[nodiscard]] std::size_t partitionCount() const noexcept
    {
        return sets_[live_.load(std::memory_order_acquire)].partitions;
    }

    void collectStateStrings(std::vector<std::pair<std::string, std::string>>& out) const override;
    void applyStateString(const std::string& key, const std::string& value) override;

private:
    /// One direction of one channel: one spectrum per partition, laid end to
    /// end so a partition is a contiguous run of bins.
    struct Spectra {
        std::vector<float> real;
        std::vector<float> imaginary;
    };

    /// A whole impulse, both ways round.
    ///
    /// Forward and reversed are both precomputed: reversing at render time
    /// would mean several hundred transforms on the audio thread, and the
    /// memory is cheaper than the rule that would break.
    struct Impulse {
        std::array<Spectra, maxChannels> forward{};
        std::array<Spectra, maxChannels> reversed{};
        std::size_t                      partitions = 0;
    };

    void adoptImpulse(std::vector<std::vector<float>> channels);
    void transformImpulse(const std::vector<std::vector<float>>& channels, bool reversed,
                          std::size_t channelCount, std::array<Spectra, maxChannels>& into);

    /// Blocks until the audio thread has certainly left the set that is about
    /// to be overwritten. Main thread only; returns immediately when nothing
    /// is rendering.
    void waitForRenderToPass() const;

    void processPartition() noexcept;

    Fft fft_;

    /// Two sets, swapped by index: a new impulse is built into the one the
    /// audio thread is NOT reading, and becomes live in a single store.
    /// Loading a file cannot then tear a block in half.
    std::array<Impulse, 2>   sets_{};
    std::atomic<std::size_t> live_{0};

    /// Bumped once per rendered block, so the loader can tell when the audio
    /// thread has moved past the set it means to reuse.
    mutable std::atomic<std::uint64_t> renderCount_{0};

    std::size_t maxPartitions_ = 0;

    /// The frequency-domain delay line: the last `partitionCount_` input
    /// blocks, newest at `fdlCursor_`.
    Spectra     fdl_{};
    std::size_t fdlCursor_ = 0;

    /// Scratch, all sized in prepare.
    std::vector<float> scratchReal_, scratchImaginary_;
    std::vector<float> accumulateReal_, accumulateImaginary_;
    std::vector<float> overlap_[maxChannels];

    /// The input half-block carried over, and the block being filled.
    std::vector<float> tail_[maxChannels];
    std::vector<float> fill_[maxChannels];
    std::size_t        filled_ = 0;

    /// Output waiting to be handed back, one partition at a time.
    std::vector<float> ready_[maxChannels];
    std::size_t        readyRead_ = 0;

    /// Pre-delay, in its own ring so it costs nothing when it is zero.
    std::vector<float> preDelay_[maxChannels];
    std::size_t        preDelayCursor_ = 0;

    /// One-pole damping and low cut on the wet.
    double dampState_[maxChannels]  = {};
    double lowCutState_[maxChannels] = {};

    SampleRate  sampleRate_ = 48000.0;
    std::string impulsePath_;
};

} // namespace incdaw::engine::dsp
