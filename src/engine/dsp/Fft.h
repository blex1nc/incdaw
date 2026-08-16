// engine/dsp/Fft.h — a small iterative radix-2 FFT.
//
// Own implementation, deliberately (D-001's "own DSP" default): the analyzer
// needs one transform size at UI resolution, which is far below the bar for
// vendoring anything. Tables (bit-reversal order, twiddles) are precomputed
// in setSize; the transform itself allocates nothing and touches no state
// but the caller's buffers, so it is realtime-safe by construction.

#pragma once

#include <cstddef>
#include <vector>

namespace incdaw::engine::dsp {

class Fft {
public:
    /// Prepares tables for `size`, which must be a power of two. Allocates —
    /// call from setup paths only.
    void setSize(std::size_t size);

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    /// In-place forward transform of `size()` complex samples split into
    /// separate real/imaginary arrays. Realtime-safe.
    void forward(float* real, float* imaginary) const noexcept;

private:
    std::size_t              size_ = 0;
    std::vector<std::size_t> reversed_;
    std::vector<float>       twiddleCos_;
    std::vector<float>       twiddleSin_;
};

} // namespace incdaw::engine::dsp
