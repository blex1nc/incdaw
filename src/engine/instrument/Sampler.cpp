#include "engine/instrument/Sampler.h"

#include <algorithm>
#include <cmath>

namespace incdaw::engine {
namespace {

/// The slice a zone plays, clamped into the sample it names.
struct Slice {
    FrameCount start = 0;
    FrameCount end   = 0;   ///< one past the last playable frame

    [[nodiscard]] bool isEmpty() const noexcept { return end <= start; }
};

/// Total playable frames: the whole file for a streamed zone, whatever is
/// decoded otherwise.
FrameCount sourceFramesOf(const SamplerZone& zone) noexcept
{
    if (zone.stream != nullptr)
        return zone.stream->fileFrames();

    return zone.sample != nullptr ? zone.sample->frameCount : 0;
}

Slice sliceOf(const SamplerZone& zone) noexcept
{
    const FrameCount frames = sourceFramesOf(zone);
    if (zone.sample == nullptr || frames <= 0)
        return {};

    Slice slice;
    slice.end   = zone.end > 0 && zone.end < frames ? zone.end : frames;
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
    sampleRate_   = sampleRate;
    maxBlockSize_ = maxBlockSize;
    allNotesOff();
    ensureStreamScratch();
}

void Sampler::allNotesOff() noexcept
{
    for (Voice& voice : voices_)
        endVoice(voice);
}

void Sampler::endVoice(Voice& voice) noexcept
{
    if (voice.streamOwner != nullptr && voice.streamSlot >= 0)
        voice.streamOwner->releaseSlot(voice.streamSlot);

    voice.streamSlot  = -1;
    voice.streamOwner = nullptr;
    voice.stage       = Stage::idle;
    voice.zone        = nullptr;
}

void Sampler::ensureStreamScratch()
{
    bool anyStreamed = false;
    for (const SamplerZone& zone : zones_)
        anyStreamed = anyStreamed || zone.stream != nullptr;

    if (!anyStreamed || maxBlockSize_ <= 0)
        return;

    // The widest span one block can consume, plus the interpolation guard.
    const std::size_t frames = static_cast<std::size_t>(
        static_cast<double>(maxBlockSize_) * maxStreamRate) + 2;

    for (Voice& voice : voices_)
        for (auto& channel : voice.scratch)
            if (channel.size() < frames)
                channel.assign(frames, Sample{0});
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
    ensureStreamScratch();
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

    // Streamed zones are forward-only by contract; the compiler preloads
    // reversed ones. A zone that violates it is refused, not half-played.
    if (zone.stream != nullptr && zone.reverse)
        return;

    Voice& voice = voices_[static_cast<std::size_t>(index)];

    // A stolen voice returns its slot before this one claims anything.
    endVoice(voice);

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
    voice.filter    = {};
    voice.lfoPhase  = 0.0;

    if (zone.stream != nullptr) {
        voice.streamOwner = zone.stream.get();
        voice.streamSlot  = zone.stream->claimSlot();

        // Steer the claimed window to the hand-over point immediately — the
        // head plays from RAM, so the stream's first useful frames are the
        // ones just past it. A one-frame read publishes the wanted position,
        // wait-free; a previous voice on this slot may have dragged the
        // window far ahead, and the head covers the catch-up time.
        if (AudioStream* stream = zone.stream->streamFor(voice.streamSlot)) {
            Sample dummy = 0;
            stream->read(zone.sample->frameCount - 1, 1, 0, &dummy);
        }
    }
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
        endVoice(voice);
        return;
    }

    const FrameCount lastFrame = slice.end - 1;

    // The sustain loop, validated rather than repaired: a loop that does not
    // fit the slice is not a loop the user drew, so it does not play. A
    // streamed zone never loops — the compiler preloads looped zones whole.
    const bool looping = zone.stream == nullptr && !zone.reverse
                      && zone.loopEnd > zone.loopStart
                      && zone.loopStart >= slice.start && zone.loopEnd <= slice.end;

    const double loopEnd    = static_cast<double>(zone.loopEnd);
    const double loopLength = static_cast<double>(zone.loopEnd - zone.loopStart);

    // The crossfade needs pre-loop material to blend with: clamp to the loop
    // length and to what exists between the slice start and loopStart.
    double crossfade = 0.0;
    if (looping && zone.loopCrossfade > 0) {
        crossfade = static_cast<double>(zone.loopCrossfade);
        crossfade = std::min(crossfade, loopLength);
        crossfade = std::min(crossfade, static_cast<double>(zone.loopStart - slice.start));
    }

    // Envelope increments, computed once per block like the reference synth.
    // A zero-or-less time means "immediately".
    const double attack  = attack_.load(std::memory_order_relaxed);
    const double decay   = decay_.load(std::memory_order_relaxed);
    const double sustain = sustain_.load(std::memory_order_relaxed);
    const double release = release_.load(std::memory_order_relaxed);

    const double attackStep  = attack > 0.0 ? 1.0 / (attack * sampleRate_) : 1.0;
    const double decayStep   = decay > 0.0 ? (1.0 - sustain) / (decay * sampleRate_) : 1.0;
    const double releaseStep = release > 0.0 ? 1.0 / (release * sampleRate_) : 1.0;

    // Filter and LFO settings, loaded once per block like the envelope. The
    // LFO only costs anything when a destination has depth.
    const int    filterMode  = filterMode_.load(std::memory_order_relaxed);
    const double cutoffHz    = filterCutoff_.load(std::memory_order_relaxed);
    const double resonance   = std::max(0.1, filterResonance_.load(std::memory_order_relaxed));
    const double lfoRate     = lfoRate_.load(std::memory_order_relaxed);
    const double lfoToPitch  = lfoToPitch_.load(std::memory_order_relaxed);
    const double lfoToCutoff = lfoToCutoff_.load(std::memory_order_relaxed);

    const bool   lfoActive = lfoRate > 0.0 && (lfoToPitch != 0.0 || lfoToCutoff != 0.0);
    const double lfoStep   = lfoRate / sampleRate_;
    const double dampening = 1.0 / resonance;

    constexpr double pi     = 3.14159265358979323846;
    constexpr double twoPi  = 2.0 * pi;

    // With no cutoff modulation the coefficient is constant for the block.
    double filterCoefficient = 0.0;
    if (filterMode != 0) {
        const double clamped = std::clamp(cutoffHz, 20.0, sampleRate_ * 0.24);
        filterCoefficient    = 2.0 * std::sin(pi * clamped / sampleRate_);
    }

    const std::size_t outputChannels = output.channelCount();
    const double      gain           = zone.gain * voice.velocity;

    // ── Streamed span fetch ──────────────────────────────────────────────────
    // The head serves positions below `headLimit` from RAM; past it, this
    // block's span is copied out of the pooled stream's window into the
    // voice's scratch, and the per-frame path interpolates locally. One
    // wait-free copy per channel per block; whatever the window cannot serve
    // arrives as silence and is counted by the stream.
    const bool  streamed      = zone.stream != nullptr;
    double      headLimit     = 0.0;
    FrameCount  fetchStart    = 0;
    bool        fetched       = false;
    std::size_t fetchChannels = 0;

    if (streamed) {
        // `sample` is the head here, its interpolation guard frame included.
        headLimit = static_cast<double>(sample.frameCount - 1);

        AudioStream* stream = zone.stream->streamFor(voice.streamSlot);

        const double pitchBound =
            lfoActive && lfoToPitch != 0.0 ? std::exp2(std::abs(lfoToPitch) / 12.0) : 1.0;
        const double spanEnd = voice.position
                             + voice.rate * pitchBound * static_cast<double>(frameCount) + 2.0;

        if (spanEnd >= headLimit) {
            fetchStart = static_cast<FrameCount>(std::max(voice.position, headLimit));

            if (stream != nullptr && !voice.scratch[0].empty()) {
                const auto scratchFrames = static_cast<FrameCount>(voice.scratch[0].size());
                const FrameCount needed =
                    static_cast<FrameCount>(spanEnd) - fetchStart + 2;
                const FrameCount count = std::min(needed, scratchFrames);

                fetchChannels =
                    std::min(sample.channelCount, SamplerZoneStream::maxSourceChannels);
                for (std::size_t channel = 0; channel < fetchChannels; ++channel)
                    stream->read(fetchStart, count, channel, voice.scratch[channel].data());

                fetched = true;
            }
        } else if (stream != nullptr) {
            // Still inside the head: keep the window parked at the hand-over.
            Sample dummy = 0;
            stream->read(sample.frameCount - 1, 1, 0, &dummy);
        }
    }

    for (FrameCount frame = 0; frame < frameCount; ++frame) {
        // The source ran out: forward past the slice, or reverse before it.
        if (voice.position < static_cast<double>(slice.start)
            || voice.position > static_cast<double>(lastFrame)) {
            endVoice(voice);
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
                    endVoice(voice);
                    return;
                }
                break;

            case Stage::idle:
                return;
        }

        const double amplitude = voice.level * gain;

        double lfo = 0.0;
        if (lfoActive) {
            lfo = std::sin(voice.lfoPhase * twoPi);
            voice.lfoPhase += lfoStep;
            if (voice.lfoPhase >= 1.0)
                voice.lfoPhase -= 1.0;
        }

        // Cutoff modulation moves the coefficient per frame; otherwise the
        // block-constant one stands.
        double f = filterCoefficient;
        if (filterMode != 0 && lfoToCutoff != 0.0) {
            const double modulated =
                std::clamp(cutoffHz * std::exp2(lfo * lfoToCutoff), 20.0, sampleRate_ * 0.24);
            f = 2.0 * std::sin(pi * modulated / sampleRate_);
        }

        // Inside the crossfade region the seam blends toward the material
        // just before loopStart — the exact content the wrap lands on, so
        // the junction is continuous by construction.
        double blend = 0.0;
        if (crossfade > 0.0 && voice.position >= loopEnd - crossfade)
            blend = (voice.position - (loopEnd - crossfade)) / crossfade;

        for (std::size_t channel = 0; channel < outputChannels; ++channel) {
            // A mono sample feeds every output channel; a multichannel one
            // maps channel for channel and repeats its last for the rest.
            const std::size_t sourceChannel =
                channel < sample.channelCount ? channel : sample.channelCount - 1;

            const std::vector<Sample>& source = sample.channels[sourceChannel];

            Sample value;

            if (!streamed) {
                value = interpolate(source, voice.position, lastFrame);
            } else if (voice.position < headLimit) {
                // RAM path: the head, clamped at its own guard frame.
                value = interpolate(source, voice.position, sample.frameCount - 1);
            } else if (fetched) {
                const double local    = voice.position - static_cast<double>(fetchStart);
                const auto   base     = static_cast<std::size_t>(local);
                const double fraction = local - static_cast<double>(base);

                const std::vector<Sample>& window =
                    voice.scratch[sourceChannel < fetchChannels ? sourceChannel
                                                                : fetchChannels - 1];

                if (base + 1 < window.size()) {
                    const double a = static_cast<double>(window[base]);
                    const double b = static_cast<double>(window[base + 1]);
                    value = static_cast<Sample>(a + (b - a) * fraction);
                } else {
                    value = 0;   // the block outran the fetch bound
                }
            } else {
                value = 0;   // no pool slot: a head-only voice past the head
            }

            if (blend > 0.0) {
                const Sample early =
                    interpolate(source, voice.position - loopLength, lastFrame);
                value = static_cast<Sample>(static_cast<double>(value) * (1.0 - blend)
                                            + static_cast<double>(early) * blend);
            }

            // Chamberlin state-variable filter, one state per channel. Runs
            // before the envelope multiply — both are linear, but the filter
            // state must see the raw signal so retriggering stays clickless.
            if (filterMode != 0 && channel < Voice::maxFilterChannels) {
                auto& state = voice.filter[channel];

                const double input = static_cast<double>(value);
                state.low += f * state.band;
                const double high = input - state.low - dampening * state.band;
                state.band += f * high;

                const double filtered = filterMode == 1   ? state.low
                                        : filterMode == 2 ? high
                                                          : state.band;
                value = static_cast<Sample>(filtered);
            }

            output.channel(channel)[frame] += static_cast<Sample>(amplitude)
                                            * value;
        }

        const double pitchFactor =
            lfoActive && lfoToPitch != 0.0 ? std::exp2(lfo * lfoToPitch / 12.0) : 1.0;

        voice.position += voice.rate * pitchFactor;

        if (looping && voice.position >= loopEnd)
            voice.position -= loopLength;
    }
}

void Sampler::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    for (Voice& voice : voices_)
        if (voice.isActive() && voice.zone != nullptr)
            renderVoice(voice, output, frameCount);
}

} // namespace incdaw::engine
