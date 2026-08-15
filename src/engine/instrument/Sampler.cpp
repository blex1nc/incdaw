#include "engine/instrument/Sampler.h"

#include <cmath>

namespace incdaw::engine {
namespace {

/// The slice a zone plays, clamped into the sample it names.
struct Slice {
    FrameCount start = 0;
    FrameCount end   = 0;   ///< one past the last playable frame

    [[nodiscard]] bool isEmpty() const noexcept { return end <= start; }
};

Slice sliceOf(const SamplerZone& zone) noexcept
{
    if (zone.sample == nullptr || zone.sample->frameCount <= 0)
        return {};

    Slice slice;
    slice.end   = zone.end > 0 && zone.end < zone.sample->frameCount ? zone.end
                                                                     : zone.sample->frameCount;
    slice.start = zone.start < slice.end ? zone.start : slice.end;
    return slice;
}

/// Linear interpolation inside one channel, clamped at the slice edge: the
/// last frame interpolates against itself rather than reading past the end.
Sample interpolate(const std::vector<Sample>& channel, double position,
                   FrameCount lastFrame) noexcept
{
    const auto   base     = static_cast<FrameCount>(position);
    const double fraction = position - static_cast<double>(base);

    const FrameCount next = base < lastFrame ? base + 1 : lastFrame;

    const double a = static_cast<double>(channel[static_cast<std::size_t>(base)]);
    const double b = static_cast<double>(channel[static_cast<std::size_t>(next)]);

    return static_cast<Sample>(a + (b - a) * fraction);
}

} // namespace

void Sampler::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate;
    allNotesOff();
}

void Sampler::allNotesOff() noexcept
{
    for (Voice& voice : voices_) {
        voice.stage = Stage::idle;
        voice.zone  = nullptr;
    }
}

int Sampler::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const Voice& voice : voices_)
        if (voice.isActive())
            ++count;

    return count;
}

void Sampler::setZones(std::vector<SamplerZone> zones)
{
    // Voices hold pointers into zones_; they cannot survive the swap. Build
    // time only, per the header contract — nothing is rendering now.
    allNotesOff();
    zones_ = std::move(zones);
}

int Sampler::findVoiceToSteal() const noexcept
{
    // A free voice first; then the quietest releasing voice; then the oldest.
    // The policy is SimpleSynth's: stealing a release is inaudible, stealing
    // the oldest is the least surprising.
    int    quietestRelease = -1;
    double quietestLevel   = 2.0;

    int           oldest        = 0;
    std::uint64_t oldestStarted = ~std::uint64_t{0};

    for (int index = 0; index < maxVoices; ++index) {
        const Voice& voice = voices_[static_cast<std::size_t>(index)];

        if (!voice.isActive())
            return index;

        if (voice.stage == Stage::release && voice.level < quietestLevel) {
            quietestLevel   = voice.level;
            quietestRelease = index;
        }

        if (voice.startedAt < oldestStarted) {
            oldestStarted = voice.startedAt;
            oldest        = index;
        }
    }

    return quietestRelease >= 0 ? quietestRelease : oldest;
}

void Sampler::startVoice(int index, const SamplerZone& zone, int channel, int key,
                         int velocity) noexcept
{
    const Slice slice = sliceOf(zone);
    if (slice.isEmpty() || sampleRate_ <= 0.0)
        return;

    Voice& voice = voices_[static_cast<std::size_t>(index)];

    const double semitones = static_cast<double>(key - zone.rootKey);
    const double rate = std::exp2(semitones / 12.0) * (zone.sample->sampleRate / sampleRate_);

    voice.stage     = Stage::attack;
    voice.key       = key;
    voice.channel   = channel;
    voice.zone      = &zone;
    voice.rate      = zone.reverse ? -rate : rate;
    voice.position  = zone.reverse ? static_cast<double>(slice.end - 1)
                                   : static_cast<double>(slice.start);
    voice.level     = 0.0;
    voice.velocity  = static_cast<double>(velocity) / 127.0;
    voice.startedAt = ++voiceCounter_;
}

void Sampler::releaseVoicesForKey(int channel, int key) noexcept
{
    for (Voice& voice : voices_)
        if (voice.isActive() && voice.stage != Stage::release && voice.key == key
            && voice.channel == channel)
            voice.stage = Stage::release;
}

void Sampler::handleMessage(const MidiMessage& message) noexcept
{
    if (message.isNoteOn()) {
        // Retriggering a sounding key releases the old voices rather than
        // stacking a second copy, which would double the level.
        releaseVoicesForKey(message.channel(), message.noteNumber());

        // Every matching zone starts a voice: that IS velocity layering.
        for (const SamplerZone& zone : zones_)
            if (zone.matches(message.noteNumber(), message.velocity()))
                startVoice(findVoiceToSteal(), zone, message.channel(), message.noteNumber(),
                           message.velocity());

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

void Sampler::renderVoice(Voice& voice, const AudioBufferView& output,
                          FrameCount frameCount) noexcept
{
    const SamplerZone&   zone   = *voice.zone;
    const AudioFileData& sample = *zone.sample;
    const Slice          slice  = sliceOf(zone);

    if (slice.isEmpty()) {
        voice.stage = Stage::idle;
        return;
    }

    const FrameCount lastFrame = slice.end - 1;

    // Envelope increments, computed once per block like the reference synth.
    // A zero-or-less time means "immediately".
    const double attack  = attack_.load(std::memory_order_relaxed);
    const double decay   = decay_.load(std::memory_order_relaxed);
    const double sustain = sustain_.load(std::memory_order_relaxed);
    const double release = release_.load(std::memory_order_relaxed);

    const double attackStep  = attack > 0.0 ? 1.0 / (attack * sampleRate_) : 1.0;
    const double decayStep   = decay > 0.0 ? (1.0 - sustain) / (decay * sampleRate_) : 1.0;
    const double releaseStep = release > 0.0 ? 1.0 / (release * sampleRate_) : 1.0;

    const std::size_t outputChannels = output.channelCount();
    const double      gain           = zone.gain * voice.velocity;

    for (FrameCount frame = 0; frame < frameCount; ++frame) {
        // The source ran out: forward past the slice, or reverse before it.
        if (voice.position < static_cast<double>(slice.start)
            || voice.position > static_cast<double>(lastFrame)) {
            voice.stage = Stage::idle;
            return;
        }

        switch (voice.stage) {
            case Stage::attack:
                voice.level += attackStep;
                if (voice.level >= 1.0) {
                    voice.level = 1.0;
                    voice.stage = Stage::decay;
                }
                break;

            case Stage::decay:
                voice.level -= decayStep;
                if (voice.level <= sustain) {
                    voice.level = sustain;
                    voice.stage = Stage::sustain;
                }
                break;

            case Stage::sustain:
                voice.level = sustain;
                break;

            case Stage::release:
                voice.level -= releaseStep;
                if (voice.level <= 0.0) {
                    voice.level = 0.0;
                    voice.stage = Stage::idle;
                    return;
                }
                break;

            case Stage::idle:
                return;
        }

        const double amplitude = voice.level * gain;

        for (std::size_t channel = 0; channel < outputChannels; ++channel) {
            // A mono sample feeds every output channel; a multichannel one
            // maps channel for channel and repeats its last for the rest.
            const std::size_t sourceChannel =
                channel < sample.channelCount ? channel : sample.channelCount - 1;

            const Sample value =
                interpolate(sample.channels[sourceChannel], voice.position, lastFrame);

            output.channel(channel)[frame] += static_cast<Sample>(amplitude)
                                            * value;
        }

        voice.position += voice.rate;
    }
}

void Sampler::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    for (Voice& voice : voices_)
        if (voice.isActive() && voice.zone != nullptr)
            renderVoice(voice, output, frameCount);
}

} // namespace incdaw::engine
