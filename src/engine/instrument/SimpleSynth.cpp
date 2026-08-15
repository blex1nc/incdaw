#include "engine/instrument/SimpleSynth.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace incdaw::engine {
namespace {

/// MIDI note to frequency. A440 at note 69, equal temperament.
double frequencyForKey(int key) noexcept
{
    return 440.0 * std::pow(2.0, (static_cast<double>(key) - 69.0) / 12.0);
}

/// PolyBLEP correction around a discontinuity.
///
/// A naive sawtooth or square steps instantaneously once per cycle, and that
/// step contains energy at every frequency — most of it above Nyquist, which
/// folds back down as inharmonic aliasing. It is clearly audible on high notes
/// and it is the single most common tell of a synth written in an afternoon.
///
/// This smooths the step over roughly one sample either side, which removes
/// most of that energy for a few lines of arithmetic.
double polyBlep(double phase, double increment) noexcept
{
    if (increment <= 0.0)
        return 0.0;

    if (phase < increment) {
        const double t = phase / increment;
        return t + t - t * t - 1.0;
    }

    if (phase > 1.0 - increment) {
        const double t = (phase - 1.0) / increment;
        return t * t + t + t + 1.0;
    }

    return 0.0;
}

} // namespace

void SimpleSynth::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    allNotesOff();
}

void SimpleSynth::allNotesOff() noexcept
{
    for (Voice& voice : voices_) {
        voice.stage = Stage::idle;
        voice.key   = -1;
        voice.level = 0.0;
        voice.phase = 0.0;
    }
}

int SimpleSynth::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const Voice& voice : voices_)
        if (voice.isActive())
            ++count;

    return count;
}

namespace {

/// voices_ is indexed by a plain int throughout (voice indices are small and
/// signed arithmetic reads better here); this keeps the conversion in one place
/// instead of scattering casts through the allocator.
constexpr std::size_t voiceSlot(int index) noexcept { return static_cast<std::size_t>(index); }

} // namespace

int SimpleSynth::findVoiceToSteal() const noexcept
{
    // An idle voice, if there is one.
    for (int index = 0; index < maxVoices; ++index)
        if (!voices_[voiceSlot(index)].isActive())
            return index;

    // Otherwise the quietest releasing voice: it is already on its way out, so
    // stealing it is the least audible choice available.
    int    best      = -1;
    double bestLevel = 1e9;

    for (int index = 0; index < maxVoices; ++index) {
        if (voices_[voiceSlot(index)].stage == Stage::release
            && voices_[voiceSlot(index)].level < bestLevel) {
            bestLevel = voices_[voiceSlot(index)].level;
            best = index;
        }
    }

    if (best >= 0)
        return best;

    // Everything is sounding: take the oldest, which is the note the player is
    // least likely to still be listening to.
    std::uint64_t oldest = ~0ull;
    best = 0;

    for (int index = 0; index < maxVoices; ++index) {
        if (voices_[voiceSlot(index)].startedAt < oldest) {
            oldest = voices_[voiceSlot(index)].startedAt;
            best = index;
        }
    }

    return best;
}

void SimpleSynth::startVoice(int index, int channel, int key, int velocity) noexcept
{
    Voice& voice = voices_[voiceSlot(index)];

    voice.stage     = Stage::attack;
    voice.key       = key;
    voice.channel   = channel;
    voice.increment = frequencyForKey(key) / sampleRate_;
    voice.velocity  = static_cast<double>(velocity) / 127.0;
    voice.startedAt = ++voiceCounter_;

    // Phase is NOT reset when stealing a sounding voice: restarting at zero
    // produces a click, because the waveform jumps from wherever it was to the
    // start of a cycle. The envelope handles the transition instead.
    if (voice.level <= 0.0)
        voice.phase = 0.0;
}

void SimpleSynth::releaseVoicesForKey(int channel, int key) noexcept
{
    for (Voice& voice : voices_) {
        if (voice.isActive() && voice.stage != Stage::release
            && voice.key == key && voice.channel == channel)
            voice.stage = Stage::release;
    }
}

void SimpleSynth::handleMessage(const MidiMessage& message) noexcept
{
    if (message.isNoteOn()) {
        // Retriggering a sounding key releases the old voice rather than
        // stacking a second one, which would double its level.
        releaseVoicesForKey(message.channel(), message.noteNumber());
        startVoice(findVoiceToSteal(), message.channel(), message.noteNumber(), message.velocity());
        return;
    }

    if (message.isNoteOff()) {
        releaseVoicesForKey(message.channel(), message.noteNumber());
        return;
    }

    if (message.isControlChange()) {
        // CC 123 is All Notes Off, and a controller that sends it expects the
        // sound to stop. Ignoring it is a classic source of stuck notes.
        if (message.data1 == 123 || message.data1 == 120)
            allNotesOff();
    }
}

double SimpleSynth::sampleWaveform(const Voice& voice, Waveform waveform) const noexcept
{
    switch (waveform) {
        case Waveform::sine:
            return std::sin(2.0 * std::numbers::pi * voice.phase);

        case Waveform::sawtooth: {
            // Naive ramp, then the band-limiting correction.
            double value = 2.0 * voice.phase - 1.0;
            value -= polyBlep(voice.phase, voice.increment);
            return value;
        }

        case Waveform::square: {
            double value = voice.phase < 0.5 ? 1.0 : -1.0;
            value += polyBlep(voice.phase, voice.increment);

            // The falling edge is half a cycle later.
            double half = voice.phase + 0.5;
            if (half >= 1.0)
                half -= 1.0;

            value -= polyBlep(half, voice.increment);
            return value;
        }

        case Waveform::triangle:
            // A triangle has no discontinuity, only a slope change, so its
            // aliasing is already far below a saw's and needs no correction.
            return 4.0 * std::abs(voice.phase - 0.5) - 1.0;
    }

    return 0.0;
}

void SimpleSynth::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    if (frameCount <= 0 || output.channelCount() == 0)
        return;

    const Waveform waveform = waveform_.load(std::memory_order_relaxed);
    const auto     gain     = static_cast<double>(gain_.load(std::memory_order_relaxed));

    const double attackRate  = 1.0 / std::max(1.0, attack_.load(std::memory_order_relaxed) * sampleRate_);
    const double decayRate   = 1.0 / std::max(1.0, decay_.load(std::memory_order_relaxed) * sampleRate_);
    const double releaseRate = 1.0 / std::max(1.0, release_.load(std::memory_order_relaxed) * sampleRate_);
    const double sustain     = sustain_.load(std::memory_order_relaxed);

    Sample* first = output.channel(0);

    for (Voice& voice : voices_) {
        if (!voice.isActive())
            continue;

        for (FrameCount frame = 0; frame < frameCount; ++frame) {
            switch (voice.stage) {
                case Stage::attack:
                    voice.level += attackRate;
                    if (voice.level >= 1.0) {
                        voice.level = 1.0;
                        voice.stage = Stage::decay;
                    }
                    break;

                case Stage::decay:
                    voice.level -= decayRate;
                    if (voice.level <= sustain) {
                        voice.level = sustain;
                        voice.stage = Stage::sustain;
                    }
                    break;

                case Stage::sustain:
                    voice.level = sustain;
                    break;

                case Stage::release:
                    voice.level -= releaseRate;
                    if (voice.level <= 0.0) {
                        voice.level = 0.0;
                        voice.stage = Stage::idle;
                        voice.key   = -1;
                    }
                    break;

                case Stage::idle:
                    break;
            }

            if (voice.stage == Stage::idle)
                break;

            first[frame] += static_cast<Sample>(
                sampleWaveform(voice, waveform) * voice.level * voice.velocity * gain);

            voice.phase += voice.increment;
            if (voice.phase >= 1.0)
                voice.phase -= std::floor(voice.phase);
        }
    }

    // Mono source, mirrored to the remaining channels. Panning belongs to the
    // mixer (Phase 10), not to every instrument.
    for (std::size_t channel = 1; channel < output.channelCount(); ++channel) {
        Sample* destination = output.channel(channel);
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            destination[frame] += first[frame];
    }
}

void SimpleSynth::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    switch (static_cast<SimpleSynthParam>(parameterId)) {
        case SimpleSynthParam::waveform:
            setWaveform(static_cast<Waveform>(
                std::clamp(static_cast<int>(plainValue + 0.5), 0, 3)));
            return;
        case SimpleSynthParam::gain:           setGain(static_cast<Sample>(plainValue)); return;
        case SimpleSynthParam::attackSeconds:  setAttackSeconds(plainValue); return;
        case SimpleSynthParam::decaySeconds:   setDecaySeconds(plainValue); return;
        case SimpleSynthParam::sustainLevel:   setSustainLevel(plainValue); return;
        case SimpleSynthParam::releaseSeconds: setReleaseSeconds(plainValue); return;
    }
}

} // namespace incdaw::engine
