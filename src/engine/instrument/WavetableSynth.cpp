#include "engine/instrument/WavetableSynth.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <numbers>

namespace incdaw::engine {
namespace {

using dsp::EffectParameter;

constexpr double pi = std::numbers::pi;

constexpr auto id(WavetableParam parameter) noexcept
{
    return static_cast<std::uint32_t>(parameter);
}

/// The descriptor table. Its ORDER is the storage order of `values_`; its ids
/// are frozen (see the enum). Ranges are what the panel's sliders travel and
/// what `setParameter` clamps to, so a preset can never carry a value the
/// user could not have dialled.
constexpr EffectParameter parameters[] = {
    {id(WavetableParam::oscATable),     "A Table",     0.0,
     static_cast<double>(wavetableCount - 1), 0.0, true},
    {id(WavetableParam::oscAPosition),  "A Position",  0.0,     1.0,     0.6667, false},
    {id(WavetableParam::oscALevel),     "A Level",     0.0,     1.0,     0.5,    false},
    {id(WavetableParam::oscADetune),    "A Detune",  -50.0,    50.0,    -4.0,    false},
    {id(WavetableParam::oscASemitones), "A Semis",   -24.0,    24.0,     0.0,    true},

    {id(WavetableParam::oscBTable),     "B Table",     0.0,
     static_cast<double>(wavetableCount - 1), 0.0, true},
    {id(WavetableParam::oscBPosition),  "B Position",  0.0,     1.0,     0.6667, false},
    {id(WavetableParam::oscBLevel),     "B Level",     0.0,     1.0,     0.0,    false},
    {id(WavetableParam::oscBDetune),    "B Detune",  -50.0,    50.0,     4.0,    false},
    {id(WavetableParam::oscBSemitones), "B Semis",   -24.0,    24.0,     0.0,    true},

    {id(WavetableParam::subLevel),      "Sub Level",   0.0,     1.0,     0.0,    false},
    {id(WavetableParam::subOctave),     "Sub Octave", -2.0,    -1.0,    -1.0,    true},
    {id(WavetableParam::subWave),       "Sub Wave",    0.0,     1.0,     0.0,    true},

    {id(WavetableParam::filterMode),      "Filter Mode", 0.0,     3.0,     1.0,    true},
    {id(WavetableParam::filterCutoffHz),  "Cutoff",     20.0, 20000.0, 12000.0,   false},
    {id(WavetableParam::filterResonance), "Resonance",   0.5,    10.0,     0.7071, false},
    {id(WavetableParam::filterKeytrack),  "Keytrack",    0.0,     1.0,     0.0,    false},

    {id(WavetableParam::ampAttack),  "Amp Attack",  0.0, 10.0, 0.005, false},
    {id(WavetableParam::ampDecay),   "Amp Decay",   0.0, 10.0, 0.15,  false},
    {id(WavetableParam::ampSustain), "Amp Sustain", 0.0,  1.0, 0.7,   false},
    {id(WavetableParam::ampRelease), "Amp Release", 0.0, 10.0, 0.2,   false},

    {id(WavetableParam::modAttack),     "Mod Attack",   0.0, 10.0, 0.005, false},
    {id(WavetableParam::modDecay),      "Mod Decay",    0.0, 10.0, 0.3,   false},
    {id(WavetableParam::modSustain),    "Mod Sustain",  0.0,  1.0, 0.0,   false},
    {id(WavetableParam::modRelease),    "Mod Release",  0.0, 10.0, 0.2,   false},
    {id(WavetableParam::modToCutoff),   "Mod > Cutoff", -8.0, 8.0, 0.0,   false},
    {id(WavetableParam::modToPosition), "Mod > Pos",    -1.0, 1.0, 0.0,   false},
    {id(WavetableParam::modToPitch),    "Mod > Pitch", -24.0, 24.0, 0.0,  false},

    {id(WavetableParam::lfo1Shape),      "LFO1 Shape",  0.0,  3.0, 0.0, true},
    {id(WavetableParam::lfo1RateHz),     "LFO1 Rate",   0.0, 20.0, 5.0, false},
    {id(WavetableParam::lfo1ToPitch),    "LFO1 > Pitch", -24.0, 24.0, 0.0, false},
    {id(WavetableParam::lfo1ToCutoff),   "LFO1 > Cutoff", -8.0, 8.0, 0.0, false},
    {id(WavetableParam::lfo1ToPosition), "LFO1 > Pos",   -1.0, 1.0, 0.0, false},

    {id(WavetableParam::lfo2Shape),      "LFO2 Shape",  0.0,  3.0, 0.0, true},
    {id(WavetableParam::lfo2RateHz),     "LFO2 Rate",   0.0, 20.0, 0.5, false},
    {id(WavetableParam::lfo2ToPitch),    "LFO2 > Pitch", -24.0, 24.0, 0.0, false},
    {id(WavetableParam::lfo2ToCutoff),   "LFO2 > Cutoff", -8.0, 8.0, 0.0, false},
    {id(WavetableParam::lfo2ToPosition), "LFO2 > Pos",   -1.0, 1.0, 0.0, false},

    {id(WavetableParam::gain), "Gain", 0.0, 1.0, 0.5, false},
};

constexpr std::size_t parameterCount = std::size(parameters);
static_assert(parameterCount == WavetableSynth::parameterCount,
              "the value storage and the descriptor table must be the same size");
constexpr std::size_t highestId      = id(WavetableParam::gain);

/// id → index, resolved at compile time. The alternative is a linear scan on
/// every automation write, which is 37 comparisons to answer a question the
/// compiler already knows.
constexpr auto indexById = [] {
    std::array<std::uint8_t, highestId + 1> map{};
    for (std::uint8_t& slot : map)
        slot = 0xFF;

    for (std::size_t index = 0; index < parameterCount; ++index)
        map[parameters[index].id] = static_cast<std::uint8_t>(index);

    return map;
}();

[[nodiscard]] constexpr std::size_t slotFor(std::uint32_t parameterId) noexcept
{
    if (parameterId > highestId)
        return parameterCount;

    const std::uint8_t slot = indexById[parameterId];
    return slot == 0xFF ? parameterCount : slot;
}

[[nodiscard]] double frequencyForKey(int key) noexcept
{
    return 440.0 * std::pow(2.0, (static_cast<double>(key) - 69.0) / 12.0);
}

[[nodiscard]] double wrapPhase(double phase) noexcept
{
    const double wrapped = phase - std::floor(phase);
    return (wrapped >= 0.0 && wrapped < 1.0) ? wrapped : 0.0;
}

[[nodiscard]] double lfoValue(WavetableSynth::LfoShape shape, double phase) noexcept
{
    switch (shape) {
        case WavetableSynth::LfoShape::sine:     return std::sin(2.0 * pi * phase);
        case WavetableSynth::LfoShape::triangle: return 4.0 * std::abs(phase - 0.5) - 1.0;
        case WavetableSynth::LfoShape::square:   return phase < 0.5 ? 1.0 : -1.0;
        case WavetableSynth::LfoShape::sawtooth: return 2.0 * phase - 1.0;
    }

    return 0.0;
}

[[nodiscard]] const Wavetable& tableAt(double index) noexcept
{
    const auto slot = static_cast<std::size_t>(
        std::clamp(index + 0.5, 0.0, static_cast<double>(wavetableCount - 1)));

    return wavetables()[std::min(slot, wavetables().size() - 1)];
}

constexpr std::size_t voiceSlot(int index) noexcept { return static_cast<std::size_t>(index); }

} // namespace

const dsp::EffectParameter* wavetableParameters() noexcept { return parameters; }
std::size_t                 wavetableParameterCount() noexcept { return parameterCount; }

WavetableSynth::WavetableSynth()
{
    for (std::size_t index = 0; index < parameterCount; ++index)
        values_[index].store(parameters[index].defaultValue, std::memory_order_relaxed);
}

void WavetableSynth::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    // Forces the tables to exist here, off the audio thread, rather than on
    // the first note.
    (void)wavetables();

    allNotesOff();
}

void WavetableSynth::allNotesOff() noexcept
{
    for (Voice& voice : voices_) {
        voice.amplitude  = {};
        voice.modulation = {};
        voice.key        = -1;
        voice.phaseA     = 0.0;
        voice.phaseB     = 0.0;
        voice.phaseSub   = 0.0;
        voice.filterLow  = 0.0;
        voice.filterBand = 0.0;
    }
}

int WavetableSynth::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const Voice& voice : voices_)
        if (voice.isActive())
            ++count;

    return count;
}

double WavetableSynth::value(std::uint32_t parameterId) const noexcept
{
    const std::size_t slot = slotFor(parameterId);
    if (slot >= parameterCount)
        return 0.0;

    return values_[slot].load(std::memory_order_relaxed);
}

void WavetableSynth::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    const std::size_t slot = slotFor(parameterId);
    if (slot >= parameterCount)
        return;

    const EffectParameter& parameter = parameters[slot];

    double clamped = std::clamp(plainValue, parameter.minValue, parameter.maxValue);
    if (parameter.stepped)
        clamped = std::round(clamped);

    values_[slot].store(clamped, std::memory_order_relaxed);
}

WavetableSynth::Settings WavetableSynth::readSettings() const noexcept
{
    const auto read = [this](WavetableParam parameter) {
        return values_[slotFor(id(parameter))].load(std::memory_order_relaxed);
    };

    Settings settings;

    settings.tableA   = &tableAt(read(WavetableParam::oscATable));
    settings.tableB   = &tableAt(read(WavetableParam::oscBTable));
    settings.tableSub = &wavetables().front();   // "Basic": sine at one end, square at the other

    settings.positionA = read(WavetableParam::oscAPosition);
    settings.positionB = read(WavetableParam::oscBPosition);
    settings.levelA    = read(WavetableParam::oscALevel);
    settings.levelB    = read(WavetableParam::oscBLevel);

    settings.tuneA = read(WavetableParam::oscASemitones)
                   + read(WavetableParam::oscADetune) / 100.0;
    settings.tuneB = read(WavetableParam::oscBSemitones)
                   + read(WavetableParam::oscBDetune) / 100.0;

    settings.levelSub    = read(WavetableParam::subLevel);
    settings.subOctave   = read(WavetableParam::subOctave);
    settings.subPosition = read(WavetableParam::subWave) >= 0.5 ? 1.0 : 0.0;

    settings.filterMode =
        static_cast<FilterMode>(static_cast<int>(read(WavetableParam::filterMode)));
    settings.cutoffHz  = read(WavetableParam::filterCutoffHz);
    settings.resonance = read(WavetableParam::filterResonance);
    settings.keytrack  = read(WavetableParam::filterKeytrack);

    settings.ampAttack  = read(WavetableParam::ampAttack);
    settings.ampDecay   = read(WavetableParam::ampDecay);
    settings.ampSustain = read(WavetableParam::ampSustain);
    settings.ampRelease = read(WavetableParam::ampRelease);

    settings.modAttack  = read(WavetableParam::modAttack);
    settings.modDecay   = read(WavetableParam::modDecay);
    settings.modSustain = read(WavetableParam::modSustain);
    settings.modRelease = read(WavetableParam::modRelease);

    settings.modToCutoff   = read(WavetableParam::modToCutoff);
    settings.modToPosition = read(WavetableParam::modToPosition);
    settings.modToPitch    = read(WavetableParam::modToPitch);

    settings.lfo1Shape =
        static_cast<LfoShape>(static_cast<int>(read(WavetableParam::lfo1Shape)));
    settings.lfo1Rate       = read(WavetableParam::lfo1RateHz);
    settings.lfo1ToPitch    = read(WavetableParam::lfo1ToPitch);
    settings.lfo1ToCutoff   = read(WavetableParam::lfo1ToCutoff);
    settings.lfo1ToPosition = read(WavetableParam::lfo1ToPosition);

    settings.lfo2Shape =
        static_cast<LfoShape>(static_cast<int>(read(WavetableParam::lfo2Shape)));
    settings.lfo2Rate       = read(WavetableParam::lfo2RateHz);
    settings.lfo2ToPitch    = read(WavetableParam::lfo2ToPitch);
    settings.lfo2ToCutoff   = read(WavetableParam::lfo2ToCutoff);
    settings.lfo2ToPosition = read(WavetableParam::lfo2ToPosition);

    settings.gain = read(WavetableParam::gain);

    return settings;
}

double WavetableSynth::pitchHeadroom(const Settings& settings) const noexcept
{
    // Every source of pitch modulation at full swing, plus the wider of the
    // two oscillator tunings. Over-estimating costs a little brightness on
    // one note; under-estimating aliases, which is the failure that matters.
    return std::abs(settings.lfo1ToPitch) + std::abs(settings.lfo2ToPitch)
         + std::abs(settings.modToPitch)
         + std::max(std::max(settings.tuneA, settings.tuneB), 0.0);
}

int WavetableSynth::findVoiceToSteal() const noexcept
{
    for (int index = 0; index < maxVoices; ++index)
        if (!voices_[voiceSlot(index)].isActive())
            return index;

    int    best      = -1;
    double bestLevel = 1e9;

    for (int index = 0; index < maxVoices; ++index) {
        const Voice& voice = voices_[voiceSlot(index)];
        if (voice.amplitude.stage == Stage::release && voice.amplitude.level < bestLevel) {
            bestLevel = voice.amplitude.level;
            best      = index;
        }
    }

    if (best >= 0)
        return best;

    std::uint64_t oldest = ~0ull;
    best = 0;

    for (int index = 0; index < maxVoices; ++index)
        if (voices_[voiceSlot(index)].startedAt < oldest) {
            oldest = voices_[voiceSlot(index)].startedAt;
            best   = index;
        }

    return best;
}

void WavetableSynth::startVoice(int index, int channel, int key, int velocity) noexcept
{
    Voice& voice = voices_[voiceSlot(index)];

    const Settings settings = readSettings();

    voice.key       = key;
    voice.channel   = channel;
    voice.frequency = frequencyForKey(key);
    voice.velocity  = static_cast<double>(velocity) / 127.0;
    voice.startedAt = ++voiceCounter_;

    voice.amplitude.stage  = Stage::attack;
    voice.modulation.stage = Stage::attack;
    voice.modulation.level = 0.0;

    // Mip levels are chosen once, at the highest pitch this patch can reach
    // from this note.
    const double headroom = pitchHeadroom(settings);
    const double topA     = voice.frequency * std::exp2((settings.tuneA + headroom) / 12.0);
    const double topB     = voice.frequency * std::exp2((settings.tuneB + headroom) / 12.0);
    const double topSub   = voice.frequency
                          * std::exp2((settings.subOctave * 12.0 + headroom) / 12.0);

    voice.levelA   = Wavetable::levelFor(topA, sampleRate_);
    voice.levelB   = Wavetable::levelFor(topB, sampleRate_);
    voice.levelSub = Wavetable::levelFor(topSub, sampleRate_);

    // Phase is not reset when stealing a sounding voice: restarting at zero
    // clicks. A silent voice starts clean, and so does its filter.
    if (voice.amplitude.level <= 0.0) {
        voice.phaseA     = 0.0;
        voice.phaseB     = 0.0;
        voice.phaseSub   = 0.0;
        voice.filterLow  = 0.0;
        voice.filterBand = 0.0;
    }
}

void WavetableSynth::releaseVoicesForKey(int channel, int key) noexcept
{
    for (Voice& voice : voices_)
        if (voice.isActive() && voice.amplitude.stage != Stage::release
            && voice.key == key && voice.channel == channel) {
            voice.amplitude.stage  = Stage::release;
            voice.modulation.stage = Stage::release;
        }
}

void WavetableSynth::handleMessage(const MidiMessage& message) noexcept
{
    if (message.isNoteOn()) {
        releaseVoicesForKey(message.channel(), message.noteNumber());
        startVoice(findVoiceToSteal(), message.channel(), message.noteNumber(),
                   message.velocity());
        return;
    }

    if (message.isNoteOff()) {
        releaseVoicesForKey(message.channel(), message.noteNumber());
        return;
    }

    if (message.isControlChange() && (message.data1 == 123 || message.data1 == 120))
        allNotesOff();
}

void WavetableSynth::advanceEnvelope(Envelope& envelope, double attackRate, double decayRate,
                                     double sustain, double releaseRate) noexcept
{
    switch (envelope.stage) {
        case Stage::attack:
            envelope.level += attackRate;
            if (envelope.level >= 1.0) {
                envelope.level = 1.0;
                envelope.stage = Stage::decay;
            }
            break;

        case Stage::decay:
            envelope.level -= decayRate;
            if (envelope.level <= sustain) {
                envelope.level = sustain;
                envelope.stage = Stage::sustain;
            }
            break;

        case Stage::sustain:
            envelope.level = sustain;
            break;

        case Stage::release:
            envelope.level -= releaseRate;
            if (envelope.level <= 0.0) {
                envelope.level = 0.0;
                envelope.stage = Stage::idle;
            }
            break;

        case Stage::idle:
            break;
    }
}

void WavetableSynth::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    if (frameCount <= 0 || output.channelCount() == 0)
        return;

    const Settings settings = readSettings();

    const double ampAttackRate  = 1.0 / std::max(1.0, settings.ampAttack * sampleRate_);
    const double ampDecayRate   = 1.0 / std::max(1.0, settings.ampDecay * sampleRate_);
    const double ampReleaseRate = 1.0 / std::max(1.0, settings.ampRelease * sampleRate_);
    const double modAttackRate  = 1.0 / std::max(1.0, settings.modAttack * sampleRate_);
    const double modDecayRate   = 1.0 / std::max(1.0, settings.modDecay * sampleRate_);
    const double modReleaseRate = 1.0 / std::max(1.0, settings.modRelease * sampleRate_);

    const double lfo1Increment = settings.lfo1Rate / sampleRate_;
    const double lfo2Increment = settings.lfo2Rate / sampleRate_;

    const double maxCutoff = sampleRate_ * 0.45;

    Sample* first = output.channel(0);

    for (Voice& voice : voices_) {
        if (!voice.isActive())
            continue;

        for (FrameCount start = 0; start < frameCount; start += controlBlock) {
            const FrameCount end = std::min<FrameCount>(start + controlBlock, frameCount);
            const auto       offset = static_cast<double>(start);

            // Each voice reads the LFOs from the block's own start phase, so
            // the oscillators stay shared and free-running without the voice
            // loop having to advance them.
            const double lfo1 = lfoValue(settings.lfo1Shape,
                                         wrapPhase(lfo1Phase_ + lfo1Increment * offset));
            const double lfo2 = lfoValue(settings.lfo2Shape,
                                         wrapPhase(lfo2Phase_ + lfo2Increment * offset));

            const double modLevel = voice.modulation.level;

            const double pitchMod = lfo1 * settings.lfo1ToPitch
                                  + lfo2 * settings.lfo2ToPitch
                                  + modLevel * settings.modToPitch;

            const double positionMod = lfo1 * settings.lfo1ToPosition
                                     + lfo2 * settings.lfo2ToPosition
                                     + modLevel * settings.modToPosition;

            const double cutoffMod = lfo1 * settings.lfo1ToCutoff
                                   + lfo2 * settings.lfo2ToCutoff
                                   + modLevel * settings.modToCutoff;

            const double incrementA =
                voice.frequency * std::exp2((settings.tuneA + pitchMod) / 12.0) / sampleRate_;
            const double incrementB =
                voice.frequency * std::exp2((settings.tuneB + pitchMod) / 12.0) / sampleRate_;
            const double incrementSub =
                voice.frequency
                * std::exp2((settings.subOctave * 12.0 + pitchMod) / 12.0) / sampleRate_;

            const double positionA = std::clamp(settings.positionA + positionMod, 0.0, 1.0);
            const double positionB = std::clamp(settings.positionB + positionMod, 0.0, 1.0);

            const double keyOffset =
                settings.keytrack * (static_cast<double>(voice.key) - 60.0) / 12.0;
            const double cutoff = std::clamp(
                settings.cutoffHz * std::exp2(keyOffset + cutoffMod), 20.0, maxCutoff);

            const double coefficient = std::min(2.0 * std::sin(pi * cutoff / sampleRate_), 1.0);
            const double damping     = 1.0 / std::max(0.5, settings.resonance);

            for (FrameCount frame = start; frame < end; ++frame) {
                advanceEnvelope(voice.amplitude, ampAttackRate, ampDecayRate,
                                settings.ampSustain, ampReleaseRate);
                advanceEnvelope(voice.modulation, modAttackRate, modDecayRate,
                                settings.modSustain, modReleaseRate);

                if (voice.amplitude.stage == Stage::idle) {
                    voice.key = -1;
                    break;
                }

                double mixed = 0.0;

                if (settings.levelA > 0.0)
                    mixed += static_cast<double>(
                                 settings.tableA->sample(positionA, voice.levelA, voice.phaseA))
                           * settings.levelA;

                if (settings.levelB > 0.0)
                    mixed += static_cast<double>(
                                 settings.tableB->sample(positionB, voice.levelB, voice.phaseB))
                           * settings.levelB;

                if (settings.levelSub > 0.0)
                    mixed += static_cast<double>(settings.tableSub->sample(
                                 settings.subPosition, voice.levelSub, voice.phaseSub))
                           * settings.levelSub;

                if (settings.filterMode != FilterMode::off) {
                    const double high = mixed - voice.filterLow - damping * voice.filterBand;
                    voice.filterBand += coefficient * high;
                    voice.filterLow  += coefficient * voice.filterBand;

                    switch (settings.filterMode) {
                        case FilterMode::lowpass:  mixed = voice.filterLow;  break;
                        case FilterMode::highpass: mixed = high;             break;
                        case FilterMode::bandpass: mixed = voice.filterBand; break;
                        case FilterMode::off:      break;
                    }
                }

                first[frame] += static_cast<Sample>(
                    mixed * voice.amplitude.level * voice.velocity * settings.gain);

                voice.phaseA   = wrapPhase(voice.phaseA + incrementA);
                voice.phaseB   = wrapPhase(voice.phaseB + incrementB);
                voice.phaseSub = wrapPhase(voice.phaseSub + incrementSub);
            }

            if (voice.amplitude.stage == Stage::idle)
                break;
        }
    }

    lfo1Phase_ = wrapPhase(lfo1Phase_ + lfo1Increment * static_cast<double>(frameCount));
    lfo2Phase_ = wrapPhase(lfo2Phase_ + lfo2Increment * static_cast<double>(frameCount));

    // Mono source, mirrored. Panning belongs to the mixer, not to every
    // instrument.
    for (std::size_t channel = 1; channel < output.channelCount(); ++channel) {
        Sample* destination = output.channel(channel);
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            destination[frame] += first[frame];
    }
}

} // namespace incdaw::engine
