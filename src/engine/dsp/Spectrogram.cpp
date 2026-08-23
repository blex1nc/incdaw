#include "engine/dsp/Spectrogram.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine::dsp {
namespace {

constexpr float floorDb = -120.0f;

float toDecibels(double magnitude) noexcept
{
    return magnitude <= 1.0e-9 ? floorDb
                               : std::max(floorDb, static_cast<float>(20.0 * std::log10(magnitude)));
}

std::pair<std::size_t, std::size_t> clampSpan(const AudioFileData& data, FramePosition from,
                                              FramePosition to) noexcept
{
    const auto limit = static_cast<FramePosition>(data.frameCount);

    const FramePosition low  = std::clamp<FramePosition>(from, 0, limit);
    const FramePosition high = std::clamp<FramePosition>(to, low, limit);

    return {static_cast<std::size_t>(low), static_cast<std::size_t>(high)};
}

/// The channels of [low, high) summed to mono.
std::vector<Sample> mixedSpan(const AudioFileData& data, std::size_t low, std::size_t high)
{
    std::vector<Sample> mixed(high - low, 0.0f);

    for (const auto& channel : data.channels) {
        const std::size_t last = std::min(high, channel.size());

        for (std::size_t index = low; index < last; ++index)
            mixed[index - low] += channel[index];
    }

    if (data.channels.size() > 1) {
        const auto scale = 1.0f / static_cast<float>(data.channels.size());
        for (Sample& value : mixed)
            value *= scale;
    }

    return mixed;
}

} // namespace

std::size_t Spectrogram::binOfFrequency(double hertz) const noexcept
{
    if (sampleRate <= 0.0 || binCount == 0)
        return 0;

    const double bin = hertz * static_cast<double>(fftSize) / sampleRate;

    return static_cast<std::size_t>(
        std::clamp(bin, 0.0, static_cast<double>(binCount - 1)) + 0.5);
}

Spectrogram buildSpectrogram(const AudioFileData& data, FramePosition from, FramePosition to,
                             std::size_t maxColumns, std::size_t fftSize)
{
    Spectrogram picture;

    const auto [low, high] = clampSpan(data, from, to);
    if (high <= low || data.channels.empty() || maxColumns == 0)
        return picture;

    const Stft stft{fftSize};

    picture.fftSize    = stft.fftSize();
    picture.sampleRate = data.sampleRate;
    picture.binCount   = stft.binCount();
    picture.startFrame = static_cast<FramePosition>(low);
    picture.frameCount = static_cast<FrameCount>(high - low);

    const std::vector<Sample> mixed = mixedSpan(data, low, high);

    // How many analysis frames there will be, so the column mapping is known
    // before the first one arrives.
    const std::size_t padded = mixed.size() + stft.fftSize() * 2;
    const std::size_t frames =
        padded >= stft.fftSize() ? (padded - stft.fftSize()) / stft.hopSize() + 1 : 0;

    if (frames == 0)
        return picture;

    picture.columns = std::min(frames, maxColumns);
    picture.decibels.assign(picture.columns * picture.binCount, floorDb);

    picture.lowestDb  = 0.0f;
    picture.highestDb = floorDb;

    stft.analyse(mixed, [&](std::size_t frameIndex, const std::vector<float>& magnitudes) {
        const std::size_t column =
            std::min(picture.columns - 1, frameIndex * picture.columns / frames);

        for (std::size_t bin = 0; bin < picture.binCount && bin < magnitudes.size(); ++bin) {
            const float value = toDecibels(static_cast<double>(magnitudes[bin]));
            float&      cell  = picture.decibels[column * picture.binCount + bin];

            // Peak-hold where several frames share a column. A mean hides
            // exactly the short events this view exists to find.
            cell = std::max(cell, value);

            picture.lowestDb  = std::min(picture.lowestDb, cell);
            picture.highestDb = std::max(picture.highestDb, cell);
        }
    });

    return picture;
}

bool spectralErase(AudioFileData& data, FramePosition from, FramePosition to, double lowHertz,
                   double highHertz, double amount, std::size_t fftSize)
{
    const auto [low, high] = clampSpan(data, from, to);
    if (high <= low || data.channels.empty())
        return false;

    if (highHertz <= lowHertz || amount <= 0.0)
        return false;

    const Stft stft{fftSize};
    const std::size_t bins = stft.binCount();

    const double perBin = data.sampleRate > 0.0
                              ? data.sampleRate / static_cast<double>(stft.fftSize())
                              : 0.0;

    if (perBin <= 0.0)
        return false;

    const auto lowBin  = static_cast<double>(lowHertz) / perBin;
    const auto highBin = static_cast<double>(highHertz) / perBin;

    if (highBin < 0.0 || lowBin > static_cast<double>(bins - 1))
        return false;

    // A few bins of cosine taper either side. A rectangular notch rings in
    // time, and the ringing is a chirp on both sides of the edit that is more
    // noticeable than whatever was removed.
    constexpr double taperBins = 3.0;

    const auto depth = static_cast<float>(std::clamp(amount, 0.0, 1.0));

    for (auto& samples : data.channels) {
        if (samples.size() < high)
            continue;

        std::vector<Sample> slice(samples.begin() + static_cast<std::ptrdiff_t>(low),
                                  samples.begin() + static_cast<std::ptrdiff_t>(high));

        const auto processed = stft.process(slice, [&](std::size_t, float* real, float* imaginary) {
            for (std::size_t bin = 0; bin < bins; ++bin) {
                const auto position = static_cast<double>(bin);

                // 1 inside the band, 0 outside, cosine across the edges.
                double inside = 0.0;

                if (position >= lowBin && position <= highBin) {
                    inside = 1.0;

                    const double toLow  = position - lowBin;
                    const double toHigh = highBin - position;
                    const double edge   = std::min(toLow, toHigh);

                    if (edge < taperBins)
                        inside = 0.5 - 0.5 * std::cos(M_PI * edge / taperBins);
                }

                if (inside <= 0.0)
                    continue;

                const auto gain = static_cast<float>(1.0 - static_cast<double>(depth) * inside);

                real[bin]      *= gain;
                imaginary[bin] *= gain;

                // The conjugate mirror, or the inverse transform is complex
                // and its imaginary half — half the amplitude — is discarded.
                if (bin > 0 && bin < bins - 1) {
                    const std::size_t mirror = stft.fftSize() - bin;
                    real[mirror]      *= gain;
                    imaginary[mirror] *= gain;
                }
            }
        });

        std::copy(processed.begin(), processed.end(),
                  samples.begin() + static_cast<std::ptrdiff_t>(low));
    }

    return true;
}

} // namespace incdaw::engine::dsp
