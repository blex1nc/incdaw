#include "engine/dsp/effects/ConvolutionReverb.h"

#include "engine/audio/WavFile.h"
#include "engine/dsp/Resampler.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>
#include <thread>

namespace incdaw::engine::dsp {
namespace {

using Convolver = ConvolutionReverbEffect;

constexpr double pi = std::numbers::pi;

constexpr EffectParameter descriptors[] = {
    {Convolver::mix,          "Mix",         0.0,     1.0,     0.0, false},
    {Convolver::preDelayMs,   "Pre-Delay",   0.0,   200.0,     0.0, false},
    {Convolver::wetGainDb,    "Wet Gain",  -24.0,    24.0,     0.0, false},
    {Convolver::decaySeconds, "Decay",       0.1,     8.0,     8.0, false},
    {Convolver::dampingHz,    "Damping",   500.0, 20000.0, 20000.0, false},
    {Convolver::lowCutHz,     "Low Cut",    20.0,  1000.0,    20.0, false},
    {Convolver::width,        "Width",       0.0,     2.0,     1.0, false},
    {Convolver::reverse,      "Reverse",     0.0,     1.0,     0.0, true},
};

constexpr std::size_t descriptorCount = std::size(descriptors);

/// Inverse transform of a conjugate-symmetric spectrum, through the forward
/// FFT the engine already has: ifft(X) = conj(fft(conj(X))) / N, and the
/// result is real. The same trick the wavetables are built with.
void inverseRealTransform(const Fft& fft, std::vector<float>& real,
                          std::vector<float>& imaginary) noexcept
{
    for (float& value : imaginary)
        value = -value;

    fft.forward(real.data(), imaginary.data());

    const float scale = 1.0f / static_cast<float>(real.size());
    for (float& value : real)
        value *= scale;
}

/// Fills the redundant half of a spectrum from the half that was computed.
void mirrorSpectrum(std::vector<float>& real, std::vector<float>& imaginary) noexcept
{
    const std::size_t size = real.size();

    for (std::size_t bin = 1; bin < size / 2; ++bin) {
        real[size - bin]      = real[bin];
        imaginary[size - bin] = -imaginary[bin];
    }

    imaginary[0]        = 0.0f;
    imaginary[size / 2] = 0.0f;
}

/// A hall from code, so a convolver with no file is not a silent insert.
///
/// Exponentially decaying noise with the top end falling away faster than the
/// bottom — which is what a room does, and what makes this sound like a space
/// rather than like a burst of static.
std::vector<std::vector<float>> generateHall(SampleRate sampleRate, double seconds)
{
    const auto frames = static_cast<std::size_t>(seconds * sampleRate);

    std::vector<std::vector<float>> channels(2);
    for (auto& channel : channels)
        channel.assign(frames, 0.0f);

    std::uint32_t state = 0x5EED1234u;
    const auto    noise = [&state]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<double>(state) * (2.0 / 4294967296.0) - 1.0;
    };

    // A short build-up before the tail: an impulse that starts at full level
    // sounds like a gate, not like a room.
    const double buildFrames = 0.012 * sampleRate;

    double lowpass[2] = {0.0, 0.0};

    for (std::size_t index = 0; index < frames; ++index) {
        const double time  = static_cast<double>(index) / sampleRate;
        const double decay = std::exp(-3.0 * time / std::max(seconds, 0.05));
        const double build = std::min(1.0, static_cast<double>(index) / buildFrames);

        // The damping coefficient opens with time, so early reflections keep
        // their top end and the tail loses it.
        const double coefficient = 0.55 - 0.35 * decay;

        for (std::size_t channel = 0; channel < 2; ++channel) {
            lowpass[channel] += coefficient * (noise() - lowpass[channel]);
            channels[channel][index] =
                static_cast<float>(lowpass[channel] * decay * build);
        }
    }

    return channels;
}

} // namespace

ConvolutionReverbEffect::ConvolutionReverbEffect()
    : BuiltinEffect(descriptors, descriptorCount)
{
}

void ConvolutionReverbEffect::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;

    const bool rateChanged = sampleRate_ != (sampleRate > 0.0 ? sampleRate : 48000.0);
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    fft_.setSize(transformSize);

    scratchReal_.assign(transformSize, 0.0f);
    scratchImaginary_.assign(transformSize, 0.0f);
    accumulateReal_.assign(transformSize, 0.0f);
    accumulateImaginary_.assign(transformSize, 0.0f);

    const auto preDelayFrames = static_cast<std::size_t>(0.201 * sampleRate_) + 1;

    for (std::size_t channel = 0; channel < maxChannels; ++channel) {
        overlap_[channel].assign(partitionSize, 0.0f);
        tail_[channel].assign(partitionSize, 0.0f);
        fill_[channel].assign(partitionSize, 0.0f);
        ready_[channel].assign(partitionSize, 0.0f);
        preDelay_[channel].assign(preDelayFrames, 0.0f);

        dampState_[channel]   = 0.0;
        lowCutState_[channel] = 0.0;
    }

    filled_          = 0;
    readyRead_       = 0;
    preDelayCursor_  = 0;
    fdlCursor_       = 0;

    // The delay line is sized for the LONGEST impulse the engine will hold,
    // once, here. Sizing it to the impulse would mean reallocating it every
    // time a file is loaded — and the audio thread reads it.
    maxPartitions_ = static_cast<std::size_t>(maxImpulseSeconds * sampleRate_)
                       / partitionSize + 2;

    fdl_.real.assign(maxPartitions_ * binCount, 0.0f);
    fdl_.imaginary.assign(maxPartitions_ * binCount, 0.0f);

    // A rate change invalidates a resampled impulse, and the generated one
    // was written for the old rate.
    if (rateChanged || partitionCount() == 0) {
        if (impulsePath_.empty() || !loadImpulse(impulsePath_))
            generateDefaultImpulse();
    }
}

void ConvolutionReverbEffect::transformImpulse(
    const std::vector<std::vector<float>>& channels, bool reversedOrder,
    std::size_t channelCount, std::array<Spectra, maxChannels>& into)
{
    const std::size_t frames     = channels[0].size();
    const std::size_t partitions = (frames + partitionSize - 1) / partitionSize;

    for (std::size_t channel = 0; channel < maxChannels; ++channel) {
        into[channel].real.assign(partitions * binCount, 0.0f);
        into[channel].imaginary.assign(partitions * binCount, 0.0f);
    }

    for (std::size_t channel = 0; channel < channelCount; ++channel) {
        for (std::size_t partition = 0; partition < partitions; ++partition) {
            std::fill(scratchReal_.begin(), scratchReal_.end(), 0.0f);
            std::fill(scratchImaginary_.begin(), scratchImaginary_.end(), 0.0f);

            for (std::size_t index = 0; index < partitionSize; ++index) {
                const std::size_t position = partition * partitionSize + index;
                if (position >= frames)
                    break;

                const std::size_t source = reversedOrder ? frames - 1 - position : position;
                scratchReal_[index] = channels[channel][source];
            }

            fft_.forward(scratchReal_.data(), scratchImaginary_.data());

            float* destinationReal = &into[channel].real[partition * binCount];
            float* destinationImag = &into[channel].imaginary[partition * binCount];

            for (std::size_t bin = 0; bin < binCount; ++bin) {
                destinationReal[bin] = scratchReal_[bin];
                destinationImag[bin] = scratchImaginary_[bin];
            }
        }
    }

    // A mono impulse feeds both sides.
    if (channelCount == 1)
        into[1] = into[0];
}

void ConvolutionReverbEffect::waitForRenderToPass() const
{
    const std::uint64_t start = renderCount_.load(std::memory_order_acquire);

    // Two blocks: one to finish whatever is in flight, one to be sure the
    // next has started on the new set. If nothing is rendering the count
    // never moves and the timeout ends it — which is the normal case at load
    // and while the transport is stopped.
    for (int spin = 0; spin < 2000; ++spin) {
        if (renderCount_.load(std::memory_order_acquire) >= start + 2)
            return;

        std::this_thread::yield();
    }
}

void ConvolutionReverbEffect::adoptImpulse(std::vector<std::vector<float>> channels)
{
    const std::size_t liveNow = live_.load(std::memory_order_acquire);
    const std::size_t target  = 1 - liveNow;

    // Whatever is about to be written must not be what is being read.
    waitForRenderToPass();

    Impulse& into = sets_[target];

    if (channels.empty() || channels[0].empty()) {
        into.partitions = 0;
        live_.store(target, std::memory_order_release);
        return;
    }

    const std::size_t channelCount = std::min(channels.size(), maxChannels);

    const auto limit = static_cast<std::size_t>(maxImpulseSeconds * sampleRate_);
    for (auto& channel : channels)
        if (channel.size() > limit)
            channel.resize(limit);

    // Unit energy, so swapping one impulse for another does not change how
    // loud the send is. Without it a long hall arrives ten decibels above a
    // short room and every mix has to be redone.
    double energy = 0.0;
    for (std::size_t channel = 0; channel < channelCount; ++channel)
        for (const float sample : channels[channel])
            energy += static_cast<double>(sample) * static_cast<double>(sample);

    if (energy > 1.0e-12) {
        const auto scale = static_cast<float>(1.0 / std::sqrt(energy));
        for (auto& channel : channels)
            for (float& sample : channel)
                sample *= scale;
    }

    const std::size_t frames = channels[0].size();

    transformImpulse(channels, false, channelCount, into.forward);
    transformImpulse(channels, true, channelCount, into.reversed);

    into.partitions =
        std::min((frames + partitionSize - 1) / partitionSize, maxPartitions_);

    // One store, and the new impulse is what the next block convolves with.
    live_.store(target, std::memory_order_release);
}

void ConvolutionReverbEffect::generateDefaultImpulse()
{
    impulsePath_.clear();

    std::vector<std::vector<float>> hall = generateHall(sampleRate_, 2.0);
    adoptImpulse(std::move(hall));
}

bool ConvolutionReverbEffect::loadImpulse(const std::filesystem::path& path)
{
    if (path.empty())
        return false;

    AudioFileData file;
    if (!WavFile::read(path.string(), file))
        return false;

    if (file.channels.empty() || file.frameCount == 0)
        return false;

    // An impulse recorded at another rate is a different room at this one.
    if (file.sampleRate > 0.0 && std::fabs(file.sampleRate - sampleRate_) > 0.5)
        file = resample(file, sampleRate_);

    std::vector<std::vector<float>> channels;
    channels.reserve(std::min(file.channels.size(), maxChannels));

    for (std::size_t channel = 0; channel < file.channels.size() && channel < maxChannels;
         ++channel)
        channels.push_back(file.channels[channel]);

    adoptImpulse(std::move(channels));

    impulsePath_ = path.string();
    return true;
}

void ConvolutionReverbEffect::collectStateStrings(
    std::vector<std::pair<std::string, std::string>>& out) const
{
    if (!impulsePath_.empty())
        out.emplace_back(impulseKey, impulsePath_);
}

void ConvolutionReverbEffect::applyStateString(const std::string& key,
                                               const std::string& value)
{
    if (key != impulseKey)
        return;

    if (value.empty()) {
        generateDefaultImpulse();
        return;
    }

    if (!loadImpulse(value)) {
        // The file is gone or unreadable. Keep the path — reconnecting the
        // drive should restore the session — and play the generated hall
        // meanwhile rather than going silent.
        const std::string wanted = value;
        generateDefaultImpulse();
        impulsePath_ = wanted;
    }
}

void ConvolutionReverbEffect::processPartition() noexcept
{
    const bool   reversedNow = valueAt(7) >= 0.5;
    const double decay       = valueAt(3);

    // Per-partition envelope. exp(-t/tau) sampled at partition boundaries is
    // a geometric sequence, so shaping the whole tail costs one exponential
    // and a multiply per partition — which is what lets Decay be an ordinary
    // automatable parameter instead of a rebuild.
    const double perPartition = std::exp(
        -3.0 * static_cast<double>(partitionSize) / (std::max(decay, 0.05) * sampleRate_));

    const Impulse& set = sets_[live_.load(std::memory_order_acquire)];
    const std::array<Spectra, maxChannels>& impulse = reversedNow ? set.reversed : set.forward;
    const std::size_t partitions = set.partitions;

    // ── One input transform. The convolver's input is mono — a stereo
    //    impulse is what makes the OUTPUT stereo — so transforming twice
    //    would be the same arithmetic done again.
    std::fill(scratchImaginary_.begin(), scratchImaginary_.end(), 0.0f);

    for (std::size_t index = 0; index < partitionSize; ++index) {
        scratchReal_[index]                 = tail_[0][index];
        scratchReal_[partitionSize + index] = fill_[0][index];
    }

    fft_.forward(scratchReal_.data(), scratchImaginary_.data());

    {
        float* fdlReal = &fdl_.real[fdlCursor_ * binCount];
        float* fdlImag = &fdl_.imaginary[fdlCursor_ * binCount];

        for (std::size_t bin = 0; bin < binCount; ++bin) {
            fdlReal[bin] = scratchReal_[bin];
            fdlImag[bin] = scratchImaginary_[bin];
        }
    }

    for (std::size_t channel = 0; channel < maxChannels; ++channel) {
        std::fill(accumulateReal_.begin(), accumulateReal_.end(), 0.0f);
        std::fill(accumulateImaginary_.begin(), accumulateImaginary_.end(), 0.0f);

        double gain = 1.0;

        for (std::size_t partition = 0; partition < partitions; ++partition) {
            // Partition 0 is the newest input block against the head of the
            // impulse; partition k reaches k blocks further back.
            const std::size_t slot =
                (fdlCursor_ + maxPartitions_ - partition) % maxPartitions_;

            const float* inputReal = &fdl_.real[slot * binCount];
            const float* inputImag = &fdl_.imaginary[slot * binCount];

            const float* irReal = &impulse[channel].real[partition * binCount];
            const float* irImag = &impulse[channel].imaginary[partition * binCount];

            const auto scale = static_cast<float>(gain);

            for (std::size_t bin = 0; bin < binCount; ++bin) {
                const float ar = inputReal[bin], ai = inputImag[bin];
                const float br = irReal[bin],    bi = irImag[bin];

                accumulateReal_[bin]      += (ar * br - ai * bi) * scale;
                accumulateImaginary_[bin] += (ar * bi + ai * br) * scale;
            }

            gain *= perPartition;
        }

        mirrorSpectrum(accumulateReal_, accumulateImaginary_);
        inverseRealTransform(fft_, accumulateReal_, accumulateImaginary_);

        // Overlap-save: the first half is the part that wrapped, and is
        // thrown away.
        for (std::size_t index = 0; index < partitionSize; ++index)
            ready_[channel][index] = accumulateReal_[partitionSize + index];
    }

    tail_[0]   = fill_[0];
    fdlCursor_ = (fdlCursor_ + 1) % std::max<std::size_t>(maxPartitions_, 1);
    readyRead_ = 0;
}

void ConvolutionReverbEffect::process(const ProcessContext& context) noexcept
{
    sumInputsInto(context);

    renderCount_.fetch_add(1, std::memory_order_release);

    const double wet = valueAt(0);

    // Mix at zero is not "inaudible", it is untouched: the convolver has one
    // partition of latency that delay compensation already removes, so a
    // bypassed instance must not also colour the signal.
    if (wet == 0.0 || partitionCount() == 0)
        return;

    const double dry        = 1.0 - wet;
    const double wetGain    = dbToGain(valueAt(2)) * wet;
    const double widthValue = valueAt(6);

    const auto preDelayFrames = static_cast<std::size_t>(
        std::clamp(valueAt(1), 0.0, 200.0) * 0.001 * sampleRate_);

    // Both filters have an explicit OFF at the end of their travel, the same
    // convention the saturator's drive follows: a one-pole at 20 kHz is still
    // a filter, and "damping all the way up" has to mean none.
    const bool   damping  = valueAt(4) < 19999.0;
    const bool   lowCutting = valueAt(5) > 20.5;

    const double dampCoefficient =
        1.0 - std::exp(-2.0 * pi * std::min(valueAt(4), sampleRate_ * 0.45) / sampleRate_);
    const double lowCutCoefficient =
        1.0 - std::exp(-2.0 * pi * std::clamp(valueAt(5), 20.0, 1000.0) / sampleRate_);

    const std::size_t channels = std::min(context.output.channelCount(), maxChannels);
    const std::size_t ring     = preDelay_[0].size();

    for (FrameCount frame = 0; frame < context.frameCount; ++frame) {
        double dryValue[maxChannels] = {};
        double wetValue[maxChannels] = {};

        for (std::size_t channel = 0; channel < channels; ++channel)
            dryValue[channel] = static_cast<double>(context.output.channel(channel)[frame]);

        // Mono in, both sides out: a stereo impulse is what widens it.
        const double monoIn =
            channels > 1 ? (dryValue[0] + dryValue[1]) * 0.5 : dryValue[0];

        // Pre-delay sits BEFORE the convolution, which is what makes it read
        // as distance rather than as a slapback on the tail.
        const std::size_t writeAt = preDelayCursor_;
        const std::size_t readAt  = (preDelayCursor_ + ring - preDelayFrames) % ring;

        preDelay_[0][writeAt] = static_cast<float>(monoIn);
        const double delayed  = static_cast<double>(preDelay_[0][readAt]);

        preDelayCursor_ = (preDelayCursor_ + 1) % ring;

        // The output is taken BEFORE this frame's input goes in, which is
        // what makes the latency exactly one partition rather than one less
        // — and delay compensation is told one partition.
        for (std::size_t channel = 0; channel < maxChannels; ++channel) {
            double value = static_cast<double>(ready_[channel][readyRead_]);

            // Damping and low cut on the WET only: the dry path must come out
            // exactly as it went in.
            if (damping) {
                dampState_[channel] += dampCoefficient * (value - dampState_[channel]);
                value = dampState_[channel];
            }

            if (lowCutting) {
                lowCutState_[channel] += lowCutCoefficient * (value - lowCutState_[channel]);
                value -= lowCutState_[channel];
            }

            wetValue[channel] = value;
        }

        ++readyRead_;
        if (readyRead_ >= partitionSize)
            readyRead_ = partitionSize - 1;   // held until the next partition lands

        fill_[0][filled_] = static_cast<float>(delayed);
        ++filled_;

        if (filled_ >= partitionSize) {
            processPartition();
            filled_ = 0;
        }

        // Width on the wet, mid/side.
        if (channels > 1) {
            const double mid  = (wetValue[0] + wetValue[1]) * 0.5;
            const double side = (wetValue[0] - wetValue[1]) * 0.5 * widthValue;

            wetValue[0] = mid + side;
            wetValue[1] = mid - side;
        }

        for (std::size_t channel = 0; channel < channels; ++channel)
            context.output.channel(channel)[frame] = static_cast<Sample>(
                dryValue[channel] * dry + wetValue[channel] * wetGain);
    }
}

} // namespace incdaw::engine::dsp
