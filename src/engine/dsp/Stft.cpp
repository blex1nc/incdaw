#include "engine/dsp/Stft.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {

Stft::Stft(std::size_t fftSize)
    : fftSize_(fftSize == 0 ? defaultFftSize : fftSize)
{
    // Round down to a power of two: the FFT requires it, and silently
    // producing a wrong-sized transform would be worse than correcting here.
    std::size_t rounded = 1;
    while (rounded * 2 <= fftSize_)
        rounded *= 2;

    fftSize_ = rounded;
    hopSize_ = std::max<std::size_t>(1, fftSize_ / overlapFactor);

    fft_.setSize(fftSize_);

    window_.resize(fftSize_);
    for (std::size_t index = 0; index < fftSize_; ++index) {
        const double phase = 2.0 * M_PI * static_cast<double>(index)
                           / static_cast<double>(fftSize_);
        window_[index] = static_cast<float>(0.5 - 0.5 * std::cos(phase));
    }

    // Applying the window twice — once on analysis, once on synthesis — means
    // the overlap-add sums the SQUARED window. Measured rather than assumed:
    // the closed form is only right when the hop divides the window evenly,
    // and a normalisation that is 2% off is a 2% gain error on every edit.
    double sum = 0.0;
    for (std::size_t index = 0; index < fftSize_; index += hopSize_)
        sum += static_cast<double>(window_[index]) * static_cast<double>(window_[index]);

    windowNormalisation_ = sum > 0.0 ? static_cast<float>(1.0 / sum) : 1.0f;
}

std::vector<Sample> Stft::process(const std::vector<Sample>& input,
                                  const FrameProcessor&      processFrame) const
{
    if (input.empty())
        return {};

    // Padded at both ends by a window, so the first and last real samples are
    // covered by as many overlapping frames as the middle. Without it the
    // reconstruction tapers at the edges, and an edit applied to a selection
    // would fade in and out of the surrounding audio.
    const std::size_t pad     = fftSize_;
    const std::size_t padded  = input.size() + pad * 2;

    std::vector<float> signal(padded, 0.0f);
    std::copy(input.begin(), input.end(), signal.begin() + static_cast<std::ptrdiff_t>(pad));

    std::vector<float> output(padded, 0.0f);
    std::vector<float> real(fftSize_), imaginary(fftSize_);

    std::size_t frameIndex = 0;

    for (std::size_t start = 0; start + fftSize_ <= padded; start += hopSize_, ++frameIndex) {
        for (std::size_t index = 0; index < fftSize_; ++index) {
            real[index]      = signal[start + index] * window_[index];
            imaginary[index] = 0.0f;
        }

        fft_.forward(real.data(), imaginary.data());

        if (processFrame)
            processFrame(frameIndex, real.data(), imaginary.data());

        fft_.inverse(real.data(), imaginary.data());

        for (std::size_t index = 0; index < fftSize_; ++index)
            output[start + index] += real[index] * window_[index] * windowNormalisation_;
    }

    std::vector<Sample> result(input.size());
    std::copy(output.begin() + static_cast<std::ptrdiff_t>(pad),
              output.begin() + static_cast<std::ptrdiff_t>(pad + input.size()), result.begin());

    return result;
}

void Stft::analyse(const std::vector<Sample>& input,
                   const std::function<void(std::size_t, const std::vector<float>&)>& observe) const
{
    if (input.empty() || !observe)
        return;

    const std::size_t pad    = fftSize_;
    const std::size_t padded = input.size() + pad * 2;

    std::vector<float> signal(padded, 0.0f);
    std::copy(input.begin(), input.end(), signal.begin() + static_cast<std::ptrdiff_t>(pad));

    std::vector<float> real(fftSize_), imaginary(fftSize_);
    std::vector<float> magnitudes(binCount());

    std::size_t frameIndex = 0;

    for (std::size_t start = 0; start + fftSize_ <= padded; start += hopSize_, ++frameIndex) {
        for (std::size_t index = 0; index < fftSize_; ++index) {
            real[index]      = signal[start + index] * window_[index];
            imaginary[index] = 0.0f;
        }

        fft_.forward(real.data(), imaginary.data());

        for (std::size_t bin = 0; bin < magnitudes.size(); ++bin)
            magnitudes[bin] = std::hypot(real[bin], imaginary[bin]);

        observe(frameIndex, magnitudes);
    }
}

} // namespace incdaw::engine::dsp
