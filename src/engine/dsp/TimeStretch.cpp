#include "engine/dsp/TimeStretch.h"

#include "engine/dsp/Resampler.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace incdaw::engine::dsp {
namespace {

constexpr double pi = 3.14159265358979323846;

/// Mono mix used for alignment decisions — every channel then follows the
/// same offsets, which is what keeps a stereo image phase-coherent.
std::vector<double> monoMixOf(const AudioFileData& source)
{
    std::vector<double> mix(static_cast<std::size_t>(source.frameCount), 0.0);

    for (const std::vector<Sample>& channel : source.channels)
        for (std::size_t frame = 0; frame < mix.size() && frame < channel.size(); ++frame)
            mix[frame] += static_cast<double>(channel[frame]);

    if (source.channelCount > 1)
        for (double& value : mix)
            value /= static_cast<double>(source.channelCount);

    return mix;
}

/// Onset positions: a 5 ms block whose energy leaps past its predecessor is
/// an attack. These are the frames transient locking protects.
std::vector<std::size_t> detectOnsets(const std::vector<double>& mix, double sampleRate)
{
    const auto blockFrames =
        std::max<std::size_t>(1, static_cast<std::size_t>(sampleRate * 0.005));

    std::vector<std::size_t> onsets;
    double                   previousEnergy = 1.0e-9;

    for (std::size_t start = 0; start + blockFrames <= mix.size(); start += blockFrames) {
        double energy = 1.0e-9;
        for (std::size_t frame = start; frame < start + blockFrames; ++frame)
            energy += mix[frame] * mix[frame];

        if (energy > previousEnergy * 9.0 && energy > 1.0e-6)
            onsets.push_back(start);

        previousEnergy = energy;
    }

    return onsets;
}

/// Normalised cross-correlation of `length` frames at two positions.
double similarityAt(const std::vector<double>& mix, std::size_t a, std::size_t b,
                    std::size_t length)
{
    double dot = 0.0, energyA = 1.0e-9, energyB = 1.0e-9;

    for (std::size_t frame = 0; frame < length; ++frame) {
        const double x = mix[a + frame];
        const double y = mix[b + frame];
        dot += x * y;
        energyA += x * x;
        energyB += y * y;
    }

    return dot / std::sqrt(energyA * energyB);
}

} // namespace

AudioFileData timeStretch(const AudioFileData& source, const StretchOptions& options)
{
    const double pitchFactor = std::pow(2.0, options.pitchSemitones / 12.0);
    const double totalRatio  = options.ratio * pitchFactor;

    if (options.ratio == 1.0 && options.pitchSemitones == 0.0)
        return source;

    if (source.frameCount == 0 || source.channelCount == 0 || source.sampleRate <= 0.0
        || totalRatio <= 0.0) {
        AudioFileData empty;
        empty.sampleRate   = source.sampleRate;
        empty.channelCount = source.channelCount;
        empty.channels.resize(source.channelCount);
        return empty;
    }

    // ── WSOLA to `totalRatio` ────────────────────────────────────────────────

    const auto window =
        std::max<std::size_t>(256, static_cast<std::size_t>(source.sampleRate * 0.050));
    const std::size_t hop     = window / 2;
    const std::size_t search  = std::max<std::size_t>(32, window / 5);
    const std::size_t overlap = hop;

    const auto inputFrames = static_cast<std::size_t>(source.frameCount);
    const auto outputFrames =
        std::max<std::size_t>(1, static_cast<std::size_t>(
                                     std::llround(static_cast<double>(inputFrames) * totalRatio)));

    AudioFileData stretched;
    stretched.sampleRate   = source.sampleRate;
    stretched.channelCount = source.channelCount;
    stretched.frameCount   = static_cast<FrameCount>(outputFrames);
    stretched.channels.assign(source.channelCount,
                              std::vector<Sample>(outputFrames, Sample{0}));

    // Inputs shorter than one window cannot be aligned; windowed-sinc
    // repitching is the honest fallback (duration lands right, pitch moves —
    // documented, and only reachable for sub-50 ms material).
    if (inputFrames < window * 2) {
        AudioFileData relabeled = source;
        relabeled.sampleRate    = source.sampleRate * totalRatio;
        AudioFileData result    = resample(relabeled, source.sampleRate);
        result.sampleRate       = source.sampleRate;
        if (options.pitchSemitones == 0.0)
            return result;

        relabeled            = result;
        relabeled.sampleRate = source.sampleRate / pitchFactor;
        result               = resample(relabeled, source.sampleRate);
        result.sampleRate    = source.sampleRate;
        return result;
    }

    const std::vector<double>      mix    = monoMixOf(source);
    const std::vector<std::size_t> onsets = detectOnsets(mix, source.sampleRate);

    std::vector<double> hann(window);
    for (std::size_t frame = 0; frame < window; ++frame)
        hann[frame] = 0.5 - 0.5 * std::cos(2.0 * pi * static_cast<double>(frame)
                                           / static_cast<double>(window));

    std::vector<double> weight(outputFrames + window, 0.0);
    std::vector<std::vector<double>> channelAccumulators(
        source.channelCount, std::vector<double>(outputFrames + window, 0.0));

    std::size_t previousStart  = 0;
    std::size_t consumedOnsets = 0;   // onsets[0..consumedOnsets) already emitted
    bool        first          = true;

    for (std::size_t outputPosition = 0; outputPosition < outputFrames; outputPosition += hop) {
        const double ideal =
            static_cast<double>(outputPosition) / totalRatio;
        std::size_t start = std::min(
            inputFrames - window,
            static_cast<std::size_t>(std::max(0.0, ideal)));

        if (first) {
            start = 0;
        } else {
            // Transient locking, forward: a compressing stretch must not hop
            // over an attack — if one lies between the last window and here,
            // snap it to the window's centre, where the Hann weight is full
            // and the hit lands at strength.
            if (consumedOnsets < onsets.size()
                && onsets[consumedOnsets] < start + hop
                && onsets[consumedOnsets] > previousStart) {
                const std::size_t onset = onsets[consumedOnsets];
                start = std::min(inputFrames - window, onset > hop ? onset - hop : 0);
            } else {
                // WSOLA search: the candidate whose opening best continues
                // the previous window's tail wins. Candidates that would
                // re-attack an onset the output already carries are barred —
                // that is what stops a slowed drum hit doubling.
                const std::size_t tail = previousStart + hop;
                if (tail + overlap <= inputFrames) {
                    const std::size_t low = start > search ? start - search : 0;
                    const std::size_t high =
                        std::min(inputFrames - window, start + search);

                    double      bestScore = -2.0;
                    std::size_t bestStart = start;

                    for (std::size_t candidate = low; candidate <= high; ++candidate) {
                        // An onset the output already carries must not open a
                        // fresh window, or a slowed hit strikes twice.
                        const bool reattacks =
                            consumedOnsets > 0
                            && onsets[consumedOnsets - 1] >= candidate
                            && onsets[consumedOnsets - 1] < candidate + hop;

                        if (reattacks)
                            continue;

                        const double score = similarityAt(mix, candidate, tail, overlap);
                        if (score > bestScore) {
                            bestScore = score;
                            bestStart = candidate;
                        }
                    }

                    start = bestStart;
                }
            }
        }

        // Only the window's weighted core counts as emitted: an onset in the
        // fading tail would be nearly inaudible there, so it stays available
        // for the next window to carry at strength.
        while (consumedOnsets < onsets.size()
               && onsets[consumedOnsets] < start + window - window / 4)
            ++consumedOnsets;

        for (std::size_t frame = 0; frame < window; ++frame) {
            const std::size_t out = outputPosition + frame;
            if (out >= weight.size())
                break;

            weight[out] += hann[frame];

            for (std::size_t channel = 0; channel < source.channelCount; ++channel)
                channelAccumulators[channel][out] +=
                    hann[frame]
                    * static_cast<double>(source.channels[channel][start + frame]);
        }

        previousStart = start;
        first         = false;
    }

    for (std::size_t channel = 0; channel < source.channelCount; ++channel)
        for (std::size_t frame = 0; frame < outputFrames; ++frame)
            stretched.channels[channel][frame] =
                weight[frame] > 1.0e-6
                    ? static_cast<Sample>(channelAccumulators[channel][frame] / weight[frame])
                    : Sample{0};

    if (options.pitchSemitones == 0.0)
        return stretched;

    // ── Pitch: play the over-stretched result back faster ────────────────────
    // The material is `pitchFactor` too long; declaring its rate higher and
    // resampling to the true rate shortens it to `ratio` and moves every
    // frequency by `pitchFactor` — the classic offline repitch.
    AudioFileData relabeled = std::move(stretched);
    relabeled.sampleRate    = source.sampleRate * pitchFactor;

    AudioFileData result = resample(relabeled, source.sampleRate);
    result.sampleRate    = source.sampleRate;
    return result;
}

} // namespace incdaw::engine::dsp
