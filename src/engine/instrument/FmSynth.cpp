#include "engine/instrument/FmSynth.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

namespace incdaw::engine {
namespace {

using dsp::EffectParameter;

constexpr double pi = std::numbers::pi;

constexpr EffectParameter parameters[] = {
    {FmParam::gain, "Gain", 0.0, 1.0, 0.5, false},

    // ── Operator 1 ─────────────────────────────────────────────────────
    {FmParam::forOperator(0, FmParam::ratio), "Op1 Ratio", 0.25, 16.0, 1.0, false},
    {FmParam::forOperator(0, FmParam::fixedHz), "Op1 Fixed Hz", 0.0, 8000.0, 0.0, false},
    {FmParam::forOperator(0, FmParam::outLevel), "Op1 Out", 0.0, 1.0, 1.0, false},
    {FmParam::forOperator(0, FmParam::attack), "Op1 Attack", 0.0, 10.0, 0.005, false},
    {FmParam::forOperator(0, FmParam::decay), "Op1 Decay", 0.0, 10.0, 0.4, false},
    {FmParam::forOperator(0, FmParam::sustain), "Op1 Sustain", 0.0, 1.0, 0.7, false},
    {FmParam::forOperator(0, FmParam::release), "Op1 Release", 0.0, 10.0, 0.3, false},
    {FmParam::forOperator(0, FmParam::detune), "Op1 Detune", -50.0, 50.0, 0.0, false},

    // ── Operator 2 ─────────────────────────────────────────────────────
    {FmParam::forOperator(1, FmParam::ratio), "Op2 Ratio", 0.25, 16.0, 1.0, false},
    {FmParam::forOperator(1, FmParam::fixedHz), "Op2 Fixed Hz", 0.0, 8000.0, 0.0, false},
    {FmParam::forOperator(1, FmParam::outLevel), "Op2 Out", 0.0, 1.0, 0.0, false},
    {FmParam::forOperator(1, FmParam::attack), "Op2 Attack", 0.0, 10.0, 0.005, false},
    {FmParam::forOperator(1, FmParam::decay), "Op2 Decay", 0.0, 10.0, 0.4, false},
    {FmParam::forOperator(1, FmParam::sustain), "Op2 Sustain", 0.0, 1.0, 0.7, false},
    {FmParam::forOperator(1, FmParam::release), "Op2 Release", 0.0, 10.0, 0.3, false},
    {FmParam::forOperator(1, FmParam::detune), "Op2 Detune", -50.0, 50.0, 0.0, false},

    // ── Operator 3 ─────────────────────────────────────────────────────
    {FmParam::forOperator(2, FmParam::ratio), "Op3 Ratio", 0.25, 16.0, 1.0, false},
    {FmParam::forOperator(2, FmParam::fixedHz), "Op3 Fixed Hz", 0.0, 8000.0, 0.0, false},
    {FmParam::forOperator(2, FmParam::outLevel), "Op3 Out", 0.0, 1.0, 0.0, false},
    {FmParam::forOperator(2, FmParam::attack), "Op3 Attack", 0.0, 10.0, 0.005, false},
    {FmParam::forOperator(2, FmParam::decay), "Op3 Decay", 0.0, 10.0, 0.4, false},
    {FmParam::forOperator(2, FmParam::sustain), "Op3 Sustain", 0.0, 1.0, 0.7, false},
    {FmParam::forOperator(2, FmParam::release), "Op3 Release", 0.0, 10.0, 0.3, false},
    {FmParam::forOperator(2, FmParam::detune), "Op3 Detune", -50.0, 50.0, 0.0, false},

    // ── Operator 4 ─────────────────────────────────────────────────────
    {FmParam::forOperator(3, FmParam::ratio), "Op4 Ratio", 0.25, 16.0, 1.0, false},
    {FmParam::forOperator(3, FmParam::fixedHz), "Op4 Fixed Hz", 0.0, 8000.0, 0.0, false},
    {FmParam::forOperator(3, FmParam::outLevel), "Op4 Out", 0.0, 1.0, 0.0, false},
    {FmParam::forOperator(3, FmParam::attack), "Op4 Attack", 0.0, 10.0, 0.005, false},
    {FmParam::forOperator(3, FmParam::decay), "Op4 Decay", 0.0, 10.0, 0.4, false},
    {FmParam::forOperator(3, FmParam::sustain), "Op4 Sustain", 0.0, 1.0, 0.7, false},
    {FmParam::forOperator(3, FmParam::release), "Op4 Release", 0.0, 10.0, 0.3, false},
    {FmParam::forOperator(3, FmParam::detune), "Op4 Detune", -50.0, 50.0, 0.0, false},

    // ── Operator 5 ─────────────────────────────────────────────────────
    {FmParam::forOperator(4, FmParam::ratio), "Op5 Ratio", 0.25, 16.0, 1.0, false},
    {FmParam::forOperator(4, FmParam::fixedHz), "Op5 Fixed Hz", 0.0, 8000.0, 0.0, false},
    {FmParam::forOperator(4, FmParam::outLevel), "Op5 Out", 0.0, 1.0, 0.0, false},
    {FmParam::forOperator(4, FmParam::attack), "Op5 Attack", 0.0, 10.0, 0.005, false},
    {FmParam::forOperator(4, FmParam::decay), "Op5 Decay", 0.0, 10.0, 0.4, false},
    {FmParam::forOperator(4, FmParam::sustain), "Op5 Sustain", 0.0, 1.0, 0.7, false},
    {FmParam::forOperator(4, FmParam::release), "Op5 Release", 0.0, 10.0, 0.3, false},
    {FmParam::forOperator(4, FmParam::detune), "Op5 Detune", -50.0, 50.0, 0.0, false},

    // ── Operator 6 ─────────────────────────────────────────────────────
    {FmParam::forOperator(5, FmParam::ratio), "Op6 Ratio", 0.25, 16.0, 1.0, false},
    {FmParam::forOperator(5, FmParam::fixedHz), "Op6 Fixed Hz", 0.0, 8000.0, 0.0, false},
    {FmParam::forOperator(5, FmParam::outLevel), "Op6 Out", 0.0, 1.0, 0.0, false},
    {FmParam::forOperator(5, FmParam::attack), "Op6 Attack", 0.0, 10.0, 0.005, false},
    {FmParam::forOperator(5, FmParam::decay), "Op6 Decay", 0.0, 10.0, 0.4, false},
    {FmParam::forOperator(5, FmParam::sustain), "Op6 Sustain", 0.0, 1.0, 0.7, false},
    {FmParam::forOperator(5, FmParam::release), "Op6 Release", 0.0, 10.0, 0.3, false},
    {FmParam::forOperator(5, FmParam::detune), "Op6 Detune", -50.0, 50.0, 0.0, false},

    // ── The matrix: how much row modulates column. The diagonal is
    //    feedback, which is why there is no feedback parameter.
    {FmParam::forRoute(0, 0), "M1>1", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(0, 1), "M1>2", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(0, 2), "M1>3", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(0, 3), "M1>4", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(0, 4), "M1>5", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(0, 5), "M1>6", 0.0, 8.0, 0.0, false},

    {FmParam::forRoute(1, 0), "M2>1", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(1, 1), "M2>2", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(1, 2), "M2>3", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(1, 3), "M2>4", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(1, 4), "M2>5", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(1, 5), "M2>6", 0.0, 8.0, 0.0, false},

    {FmParam::forRoute(2, 0), "M3>1", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(2, 1), "M3>2", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(2, 2), "M3>3", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(2, 3), "M3>4", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(2, 4), "M3>5", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(2, 5), "M3>6", 0.0, 8.0, 0.0, false},

    {FmParam::forRoute(3, 0), "M4>1", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(3, 1), "M4>2", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(3, 2), "M4>3", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(3, 3), "M4>4", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(3, 4), "M4>5", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(3, 5), "M4>6", 0.0, 8.0, 0.0, false},

    {FmParam::forRoute(4, 0), "M5>1", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(4, 1), "M5>2", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(4, 2), "M5>3", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(4, 3), "M5>4", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(4, 4), "M5>5", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(4, 5), "M5>6", 0.0, 8.0, 0.0, false},

    {FmParam::forRoute(5, 0), "M6>1", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(5, 1), "M6>2", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(5, 2), "M6>3", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(5, 3), "M6>4", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(5, 4), "M6>5", 0.0, 8.0, 0.0, false},
    {FmParam::forRoute(5, 5), "M6>6", 0.0, 8.0, 0.0, false},

};
constexpr std::size_t parameterCount = std::size(parameters);
static_assert(parameterCount == FmSynth::parameterCount,
              "the value storage and the descriptor table must be the same size");

constexpr std::uint32_t highestId =
    FmParam::forRoute(FmSynth::operatorCount - 1, FmSynth::operatorCount - 1);

/// id → index, resolved at compile time: an automation write must not pay for
/// a linear scan of 85 rows.
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

/// A sine, by table. Six operators per voice times sixteen voices times a
/// block of samples is far too many calls for std::sin: a 4096-point table
/// read with linear interpolation is about -95 dB accurate, which is below
/// anything the rest of the signal path resolves.
constexpr std::size_t sineTableSize = 4096;

[[nodiscard]] const std::array<float, sineTableSize + 1>& sineTable()
{
    static const auto table = [] {
        std::array<float, sineTableSize + 1> values{};
        for (std::size_t index = 0; index <= sineTableSize; ++index)
            values[index] = static_cast<float>(
                std::sin(2.0 * pi * static_cast<double>(index)
                         / static_cast<double>(sineTableSize)));

        return values;
    }();

    return table;
}

/// `phase` in cycles, any magnitude — phase modulation routinely pushes it
/// well outside [0,1), and a NaN increment must produce silence rather than
/// an out-of-range read.
[[nodiscard]] double fastSine(double phase) noexcept
{
    const double wrapped = phase - std::floor(phase);
    if (!(wrapped >= 0.0 && wrapped < 1.0))
        return 0.0;

    const double exact = wrapped * static_cast<double>(sineTableSize);
    const auto   index = static_cast<std::size_t>(exact);
    const double frac  = exact - static_cast<double>(index);

    const std::array<float, sineTableSize + 1>& table = sineTable();
    const double low  = static_cast<double>(table[index]);
    const double high = static_cast<double>(table[index + 1]);

    return low + (high - low) * frac;
}

[[nodiscard]] double frequencyForKey(int key) noexcept
{
    return 440.0 * std::pow(2.0, (static_cast<double>(key) - 69.0) / 12.0);
}

constexpr std::size_t voiceSlot(int index) noexcept { return static_cast<std::size_t>(index); }

} // namespace

const dsp::EffectParameter* fmParameters() noexcept { return parameters; }
std::size_t                 fmParameterCount() noexcept { return parameterCount; }

FmSynth::FmSynth()
{
    for (std::size_t index = 0; index < parameterCount; ++index)
        values_[index].store(parameters[index].defaultValue, std::memory_order_relaxed);
}

void FmSynth::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;

    (void)sineTable();   // built here, off the audio thread
    allNotesOff();
}

void FmSynth::allNotesOff() noexcept
{
    for (Voice& voice : voices_) {
        voice.key = -1;
        for (OperatorState& state : voice.operators)
            state = {};
    }
}

int FmSynth::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const Voice& voice : voices_)
        if (voice.isActive())
            ++count;

    return count;
}

double FmSynth::value(std::uint32_t parameterId) const noexcept
{
    const std::size_t slot = slotFor(parameterId);
    if (slot >= parameterCount)
        return 0.0;

    return values_[slot].load(std::memory_order_relaxed);
}

void FmSynth::setParameter(std::uint32_t parameterId, double plainValue) noexcept
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

FmSynth::Settings FmSynth::readSettings() const noexcept
{
    const auto read = [this](std::uint32_t parameterId) {
        return values_[slotFor(parameterId)].load(std::memory_order_relaxed);
    };

    Settings settings;
    settings.gain = read(FmParam::gain);

    for (int index = 0; index < operatorCount; ++index) {
        OperatorSettings& op = settings.operators[static_cast<std::size_t>(index)];

        op.ratio    = read(FmParam::forOperator(index, FmParam::ratio));
        op.fixedHz  = read(FmParam::forOperator(index, FmParam::fixedHz));
        op.detune   = read(FmParam::forOperator(index, FmParam::detune));
        op.outLevel = read(FmParam::forOperator(index, FmParam::outLevel));
        op.sustain  = read(FmParam::forOperator(index, FmParam::sustain));

        op.attackRate =
            1.0 / std::max(1.0, read(FmParam::forOperator(index, FmParam::attack)) * sampleRate_);
        op.decayRate =
            1.0 / std::max(1.0, read(FmParam::forOperator(index, FmParam::decay)) * sampleRate_);
        op.releaseRate =
            1.0 / std::max(1.0, read(FmParam::forOperator(index, FmParam::release)) * sampleRate_);

        if (op.outLevel > 0.0)
            settings.anyOutput = true;
    }

    for (int source = 0; source < operatorCount; ++source)
        for (int destination = 0; destination < operatorCount; ++destination)
            settings.matrix[static_cast<std::size_t>(source * operatorCount + destination)] =
                read(FmParam::forRoute(source, destination));

    return settings;
}

int FmSynth::findVoiceToSteal() const noexcept
{
    for (int index = 0; index < maxVoices; ++index)
        if (!voices_[voiceSlot(index)].isActive())
            return index;

    // The quietest releasing voice, measured on the operators that are
    // actually heard rather than on all six.
    int    best      = -1;
    double bestLevel = 1e9;

    for (int index = 0; index < maxVoices; ++index) {
        const Voice& voice = voices_[voiceSlot(index)];

        bool   releasing = true;
        double loudest   = 0.0;

        for (const OperatorState& state : voice.operators) {
            if (state.stage != Stage::idle && state.stage != Stage::release)
                releasing = false;

            loudest = std::max(loudest, state.level);
        }

        if (releasing && loudest < bestLevel) {
            bestLevel = loudest;
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

void FmSynth::startVoice(int index, int channel, int key, int velocity) noexcept
{
    Voice& voice = voices_[voiceSlot(index)];

    const bool wasSilent = !voice.isActive();

    voice.key       = key;
    voice.channel   = channel;
    voice.frequency = frequencyForKey(key);
    voice.velocity  = static_cast<double>(velocity) / 127.0;
    voice.startedAt = ++voiceCounter_;

    for (OperatorState& state : voice.operators) {
        state.stage = Stage::attack;

        // Phases reset only on a voice that was silent. Restarting a sounding
        // voice's operators at zero is a click, and in FM it is a loud one:
        // every operator would line up in phase for an instant.
        if (wasSilent) {
            state.phase    = 0.0;
            state.previous = 0.0;
            state.level    = 0.0;
        }
    }
}

void FmSynth::releaseVoicesForKey(int channel, int key) noexcept
{
    for (Voice& voice : voices_) {
        if (!voice.isActive() || voice.key != key || voice.channel != channel)
            continue;

        for (OperatorState& state : voice.operators)
            if (state.stage != Stage::idle)
                state.stage = Stage::release;
    }
}

void FmSynth::handleMessage(const MidiMessage& message) noexcept
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

void FmSynth::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    if (frameCount <= 0 || output.channelCount() == 0)
        return;

    const Settings settings = readSettings();
    Sample*        first    = output.channel(0);

    for (Voice& voice : voices_) {
        if (!voice.isActive())
            continue;

        // Increments are per voice and constant across the block: FM's pitch
        // does not move unless the note does.
        std::array<double, operatorCount> increment{};
        for (int index = 0; index < operatorCount; ++index) {
            const OperatorSettings& op = settings.operators[static_cast<std::size_t>(index)];

            const double base = op.fixedHz > 0.0 ? op.fixedHz : voice.frequency * op.ratio;
            increment[static_cast<std::size_t>(index)] =
                base * std::exp2(op.detune / 1200.0) / sampleRate_;
        }

        for (FrameCount frame = 0; frame < frameCount; ++frame) {
            std::array<double, operatorCount> produced{};

            bool stillSounding = false;

            for (int index = 0; index < operatorCount; ++index) {
                const auto              slot = static_cast<std::size_t>(index);
                const OperatorSettings& op   = settings.operators[slot];
                OperatorState&          state = voice.operators[slot];

                switch (state.stage) {
                    case Stage::attack:
                        state.level += op.attackRate;
                        if (state.level >= 1.0) {
                            state.level = 1.0;
                            state.stage = Stage::decay;
                        }
                        break;

                    case Stage::decay:
                        state.level -= op.decayRate;
                        if (state.level <= op.sustain) {
                            state.level = op.sustain;
                            state.stage = Stage::sustain;
                        }
                        break;

                    case Stage::sustain:
                        state.level = op.sustain;
                        break;

                    case Stage::release:
                        state.level -= op.releaseRate;
                        if (state.level <= 0.0) {
                            state.level = 0.0;
                            state.stage = Stage::idle;
                        }
                        break;

                    case Stage::idle:
                        state.level = 0.0;
                        break;
                }

                if (state.stage != Stage::idle)
                    stillSounding = true;

                // Modulators are read from the PREVIOUS sample, which is what
                // makes a cyclic matrix — feedback included — computable in
                // one pass.
                double modulation = 0.0;
                for (int source = 0; source < operatorCount; ++source) {
                    const double amount = settings.matrix[
                        static_cast<std::size_t>(source * operatorCount + index)];

                    if (amount != 0.0)
                        modulation +=
                            amount * voice.operators[static_cast<std::size_t>(source)].previous;
                }

                state.phase += increment[slot];
                if (state.phase >= 1.0)
                    state.phase -= std::floor(state.phase);

                produced[slot] = fastSine(state.phase + modulation) * state.level;
            }

            double mixed = 0.0;
            for (int index = 0; index < operatorCount; ++index) {
                const auto slot = static_cast<std::size_t>(index);
                voice.operators[slot].previous = produced[slot];
                mixed += produced[slot] * settings.operators[slot].outLevel;
            }

            first[frame] += static_cast<Sample>(mixed * voice.velocity * settings.gain);

            if (!stillSounding) {
                voice.key = -1;
                break;
            }
        }
    }

    for (std::size_t channel = 1; channel < output.channelCount(); ++channel) {
        Sample* destination = output.channel(channel);
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            destination[frame] += first[frame];
    }
}

} // namespace incdaw::engine
