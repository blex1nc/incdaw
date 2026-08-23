#pragma once

#include "engine/audio/WavFile.h"
#include "engine/dsp/Stft.h"

#include <cstddef>
#include <vector>

namespace incdaw::engine::dsp {

/// A region of audio as time against frequency.
///
/// The editor's second way of looking at the same samples. A waveform shows
/// when something is loud; this shows what it is made of, which is the only
/// view in which a single squeak, a mains hum or a chair creak is a thing you
/// can point at rather than a smudge in the envelope.
///
/// Magnitudes are in decibels because that is how the ear hears them and how
/// the picture becomes legible: on a linear scale everything below the loudest
/// few bins is black.
struct Spectrogram {
    std::size_t columns  = 0;   ///< time steps
    std::size_t binCount = 0;   ///< frequency steps, DC to Nyquist

    std::size_t fftSize    = 0;
    SampleRate  sampleRate = 0.0;

    /// The span of the source this covers, in frames.
    FramePosition startFrame = 0;
    FrameCount    frameCount = 0;

    /// `columns * binCount`, column-major by time: index `column * binCount + bin`.
    std::vector<float> decibels;

    /// The range actually present, for mapping onto ink without clipping the
    /// picture to a constant nobody chose.
    float lowestDb  = 0.0f;
    float highestDb = 0.0f;

    [[nodiscard]] bool isEmpty() const noexcept { return decibels.empty(); }

    [[nodiscard]] float at(std::size_t column, std::size_t bin) const noexcept
    {
        const std::size_t index = column * binCount + bin;
        return index < decibels.size() ? decibels[index] : lowestDb;
    }

    /// The frequency at the centre of `bin`, in Hz.
    [[nodiscard]] double frequencyOfBin(std::size_t bin) const noexcept
    {
        return fftSize > 0 ? sampleRate * static_cast<double>(bin) / static_cast<double>(fftSize)
                           : 0.0;
    }

    /// The bin a frequency falls in, clamped.
    [[nodiscard]] std::size_t binOfFrequency(double hertz) const noexcept;
};

/// Analyses [from, to) of every channel, mixed, into at most `maxColumns`
/// time steps.
///
/// Capped rather than one column per hop: a five-minute region at a 512-sample
/// hop is 28,000 columns, and a picture with more columns than the screen has
/// pixels costs the memory of the audio it describes to say nothing more.
/// Columns beyond the cap are combined by taking the LOUDEST of what they
/// cover — a peak-hold, the same choice the waveform overview makes, because a
/// mean hides exactly the short events this view exists to find.
[[nodiscard]] Spectrogram buildSpectrogram(const AudioFileData& data, FramePosition from,
                                           FramePosition to,
                                           std::size_t   maxColumns = 2048,
                                           std::size_t   fftSize    = Stft::defaultFftSize);

/// Attenuates everything between `lowHertz` and `highHertz` across [from, to).
///
/// `amount` is how much: 1 removes the band, 0 does nothing. The band's edges
/// are tapered over a few bins — a rectangular notch in the spectrum rings in
/// time, and the ringing is a chirp either side of the edit that is more
/// noticeable than whatever was removed.
///
/// False when the region or the band is empty. Offline.
[[nodiscard]] bool spectralErase(AudioFileData& data, FramePosition from, FramePosition to,
                                 double lowHertz, double highHertz, double amount = 1.0,
                                 std::size_t fftSize = Stft::defaultFftSize);

} // namespace incdaw::engine::dsp
