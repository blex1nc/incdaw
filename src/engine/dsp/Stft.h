#pragma once

#include "engine/core/Time.h"
#include "engine/dsp/Fft.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace incdaw::engine::dsp {

/// Short-time Fourier analysis and resynthesis, offline.
///
/// The shared floor under denoise and spectral editing: both are "take the
/// signal apart into time and frequency, change something, put it back". Doing
/// that twice, once per feature, is how two spectral tools end up disagreeing
/// about what a bin means.
///
/// **Offline only.** It allocates, and it processes a whole region rather than
/// a block; the audio thread never sees it. That is the deliberate opposite of
/// engine/dsp/TimeStretch.h's realtime constraint, and the reason the two do
/// not share code.
///
/// **The reconstruction is exact.** A Hann window applied on analysis AND on
/// synthesis sums, at 75% overlap, to a constant — so a pass that changes no
/// bin returns the input, not a scaled or amplitude-modulated version of it.
/// That property is worth stating because it is the one every spectral tool
/// silently gets wrong: the artefact is a faint tremolo at the hop rate, which
/// sounds like "the algorithm" rather than like a bug.
class Stft {
public:
    /// 2048 at 48 kHz is ~43 ms and ~23 Hz per bin: long enough to separate a
    /// noise floor from a note, short enough that a transient does not smear
    /// across the whole window.
    static constexpr std::size_t defaultFftSize = 2048;

    /// A quarter of the window. Three-quarters overlap is what makes the
    /// squared Hann sum flat, and it is also what keeps a modified frame from
    /// being audible on its own.
    static constexpr std::size_t overlapFactor = 4;

    explicit Stft(std::size_t fftSize = defaultFftSize);

    [[nodiscard]] std::size_t fftSize() const noexcept { return fftSize_; }
    [[nodiscard]] std::size_t hopSize() const noexcept { return hopSize_; }
    [[nodiscard]] std::size_t binCount() const noexcept { return fftSize_ / 2 + 1; }

    /// The frequency at the centre of `bin`, in Hz.
    [[nodiscard]] double frequencyOfBin(std::size_t bin, SampleRate sampleRate) const noexcept
    {
        return sampleRate * static_cast<double>(bin) / static_cast<double>(fftSize_);
    }

    /// Called once per frame with that frame's half-spectrum, in place.
    ///
    /// `frameIndex` counts hops from the start of the processed span, so a
    /// caller that needs "when" as well as "what" — spectral editing does —
    /// has it without keeping its own counter.
    using FrameProcessor = std::function<void(std::size_t frameIndex, float* real, float* imaginary)>;

    /// Analyses `input`, hands every frame to `process`, and resynthesises.
    ///
    /// The result has exactly `input.size()` samples: the padding the overlap
    /// needs at both ends is added and removed internally, so a caller splices
    /// the result back over the region it came from without arithmetic.
    [[nodiscard]] std::vector<Sample> process(const std::vector<Sample>& input,
                                              const FrameProcessor&      process) const;

    /// Analyses only: hands every frame's magnitudes to `observe`. Used to
    /// learn a noise profile and to draw a spectrogram, neither of which
    /// resynthesises anything.
    void analyse(const std::vector<Sample>& input,
                 const std::function<void(std::size_t frameIndex,
                                          const std::vector<float>& magnitudes)>& observe) const;

private:
    std::size_t        fftSize_ = defaultFftSize;
    std::size_t        hopSize_ = defaultFftSize / overlapFactor;
    Fft                fft_;
    std::vector<float> window_;

    /// Sum of the squared window at the hop, used to normalise the overlap-add
    /// exactly rather than by the textbook constant — which is only correct
    /// when the window length divides evenly by the hop.
    float windowNormalisation_ = 1.0f;
};

} // namespace incdaw::engine::dsp
