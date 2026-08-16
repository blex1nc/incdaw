#include "engine/dsp/Fft.h"

#include <cmath>

namespace incdaw::engine::dsp {

void Fft::setSize(std::size_t size)
{
    size_ = size;

    reversed_.assign(size, 0);

    std::size_t bits = 0;
    while ((std::size_t{1} << bits) < size)
        ++bits;

    for (std::size_t index = 0; index < size; ++index) {
        std::size_t value = index;
        std::size_t out   = 0;
        for (std::size_t bit = 0; bit < bits; ++bit) {
            out = (out << 1) | (value & 1);
            value >>= 1;
        }
        reversed_[index] = out;
    }

    // One twiddle per element of the half-spectrum, shared by every stage.
    twiddleCos_.assign(size / 2, 0.0f);
    twiddleSin_.assign(size / 2, 0.0f);

    for (std::size_t index = 0; index < size / 2; ++index) {
        const double angle = -2.0 * M_PI * static_cast<double>(index)
                           / static_cast<double>(size);
        twiddleCos_[index] = static_cast<float>(std::cos(angle));
        twiddleSin_[index] = static_cast<float>(std::sin(angle));
    }
}

void Fft::forward(float* real, float* imaginary) const noexcept
{
    const std::size_t size = size_;

    for (std::size_t index = 0; index < size; ++index) {
        const std::size_t swapped = reversed_[index];
        if (swapped > index) {
            std::swap(real[index], real[swapped]);
            std::swap(imaginary[index], imaginary[swapped]);
        }
    }

    for (std::size_t length = 2; length <= size; length <<= 1) {
        const std::size_t half   = length / 2;
        const std::size_t stride = size / length;

        for (std::size_t start = 0; start < size; start += length) {
            for (std::size_t offset = 0; offset < half; ++offset) {
                const float cosine = twiddleCos_[offset * stride];
                const float sine   = twiddleSin_[offset * stride];

                const std::size_t even = start + offset;
                const std::size_t odd  = even + half;

                const float rotatedReal =
                    real[odd] * cosine - imaginary[odd] * sine;
                const float rotatedImaginary =
                    real[odd] * sine + imaginary[odd] * cosine;

                real[odd]      = real[even] - rotatedReal;
                imaginary[odd] = imaginary[even] - rotatedImaginary;
                real[even] += rotatedReal;
                imaginary[even] += rotatedImaginary;
            }
        }
    }
}

} // namespace incdaw::engine::dsp
