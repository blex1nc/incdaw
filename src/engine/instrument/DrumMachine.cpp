#include "engine/instrument/DrumMachine.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numbers>

namespace incdaw::engine {
namespace {

using dsp::EffectParameter;

constexpr double pi = std::numbers::pi;

constexpr EffectParameter parameters[] = {
    {DrumParam::gain, "Gain", 0.0, 1.0, 0.7, false},

    // ── Pad 1 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(0, DrumParam::engine), "P1 Engine", 0.0, 5.0, 0.0, true},
    {DrumParam::forPad(0, DrumParam::tune), "P1 Tune", -24.0, 24.0, 0.0, false},
    {DrumParam::forPad(0, DrumParam::decay), "P1 Decay", 0.01, 4.0, 0.55, false},
    {DrumParam::forPad(0, DrumParam::tone), "P1 Tone", 0.0, 1.0, 0.35, false},
    {DrumParam::forPad(0, DrumParam::level), "P1 Level", 0.0, 1.0, 0.95, false},
    {DrumParam::forPad(0, DrumParam::pan), "P1 Pan", -1.0, 1.0, 0.0, false},
    {DrumParam::forPad(0, DrumParam::chokeGroup), "P1 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(0, DrumParam::snap), "P1 Snap", 0.0, 1.0, 0.7, false},

    // ── Pad 2 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(1, DrumParam::engine), "P2 Engine", 0.0, 5.0, 0.0, true},
    {DrumParam::forPad(1, DrumParam::tune), "P2 Tune", -24.0, 24.0, -3.0, false},
    {DrumParam::forPad(1, DrumParam::decay), "P2 Decay", 0.01, 4.0, 0.75, false},
    {DrumParam::forPad(1, DrumParam::tone), "P2 Tone", 0.0, 1.0, 0.25, false},
    {DrumParam::forPad(1, DrumParam::level), "P2 Level", 0.0, 1.0, 0.85, false},
    {DrumParam::forPad(1, DrumParam::pan), "P2 Pan", -1.0, 1.0, 0.0, false},
    {DrumParam::forPad(1, DrumParam::chokeGroup), "P2 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(1, DrumParam::snap), "P2 Snap", 0.0, 1.0, 0.5, false},

    // ── Pad 3 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(2, DrumParam::engine), "P3 Engine", 0.0, 5.0, 1.0, true},
    {DrumParam::forPad(2, DrumParam::tune), "P3 Tune", -24.0, 24.0, 0.0, false},
    {DrumParam::forPad(2, DrumParam::decay), "P3 Decay", 0.01, 4.0, 0.22, false},
    {DrumParam::forPad(2, DrumParam::tone), "P3 Tone", 0.0, 1.0, 0.55, false},
    {DrumParam::forPad(2, DrumParam::level), "P3 Level", 0.0, 1.0, 0.85, false},
    {DrumParam::forPad(2, DrumParam::pan), "P3 Pan", -1.0, 1.0, 0.0, false},
    {DrumParam::forPad(2, DrumParam::chokeGroup), "P3 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(2, DrumParam::snap), "P3 Snap", 0.0, 1.0, 0.6, false},

    // ── Pad 4 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(3, DrumParam::engine), "P4 Engine", 0.0, 5.0, 1.0, true},
    {DrumParam::forPad(3, DrumParam::tune), "P4 Tune", -24.0, 24.0, 4.0, false},
    {DrumParam::forPad(3, DrumParam::decay), "P4 Decay", 0.01, 4.0, 0.12, false},
    {DrumParam::forPad(3, DrumParam::tone), "P4 Tone", 0.0, 1.0, 0.75, false},
    {DrumParam::forPad(3, DrumParam::level), "P4 Level", 0.0, 1.0, 0.7, false},
    {DrumParam::forPad(3, DrumParam::pan), "P4 Pan", -1.0, 1.0, 0.12, false},
    {DrumParam::forPad(3, DrumParam::chokeGroup), "P4 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(3, DrumParam::snap), "P4 Snap", 0.0, 1.0, 0.8, false},

    // ── Pad 5 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(4, DrumParam::engine), "P5 Engine", 0.0, 5.0, 3.0, true},
    {DrumParam::forPad(4, DrumParam::tune), "P5 Tune", -24.0, 24.0, 0.0, false},
    {DrumParam::forPad(4, DrumParam::decay), "P5 Decay", 0.01, 4.0, 0.3, false},
    {DrumParam::forPad(4, DrumParam::tone), "P5 Tone", 0.0, 1.0, 0.6, false},
    {DrumParam::forPad(4, DrumParam::level), "P5 Level", 0.0, 1.0, 0.7, false},
    {DrumParam::forPad(4, DrumParam::pan), "P5 Pan", -1.0, 1.0, -0.18, false},
    {DrumParam::forPad(4, DrumParam::chokeGroup), "P5 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(4, DrumParam::snap), "P5 Snap", 0.0, 1.0, 0.5, false},

    // ── Pad 6 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(5, DrumParam::engine), "P6 Engine", 0.0, 5.0, 5.0, true},
    {DrumParam::forPad(5, DrumParam::tune), "P6 Tune", -24.0, 24.0, 0.0, false},
    {DrumParam::forPad(5, DrumParam::decay), "P6 Decay", 0.01, 4.0, 0.09, false},
    {DrumParam::forPad(5, DrumParam::tone), "P6 Tone", 0.0, 1.0, 0.7, false},
    {DrumParam::forPad(5, DrumParam::level), "P6 Level", 0.0, 1.0, 0.6, false},
    {DrumParam::forPad(5, DrumParam::pan), "P6 Pan", -1.0, 1.0, 0.22, false},
    {DrumParam::forPad(5, DrumParam::chokeGroup), "P6 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(5, DrumParam::snap), "P6 Snap", 0.0, 1.0, 0.8, false},

    // ── Pad 7 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(6, DrumParam::engine), "P7 Engine", 0.0, 5.0, 2.0, true},
    {DrumParam::forPad(6, DrumParam::tune), "P7 Tune", -24.0, 24.0, 0.0, false},
    {DrumParam::forPad(6, DrumParam::decay), "P7 Decay", 0.01, 4.0, 0.06, false},
    {DrumParam::forPad(6, DrumParam::tone), "P7 Tone", 0.0, 1.0, 0.7, false},
    {DrumParam::forPad(6, DrumParam::level), "P7 Level", 0.0, 1.0, 0.65, false},
    {DrumParam::forPad(6, DrumParam::pan), "P7 Pan", -1.0, 1.0, 0.1, false},
    {DrumParam::forPad(6, DrumParam::chokeGroup), "P7 Choke", 0.0, 8.0, 1.0, true},
    {DrumParam::forPad(6, DrumParam::snap), "P7 Snap", 0.0, 1.0, 0.4, false},

    // ── Pad 8 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(7, DrumParam::engine), "P8 Engine", 0.0, 5.0, 2.0, true},
    {DrumParam::forPad(7, DrumParam::tune), "P8 Tune", -24.0, 24.0, 0.0, false},
    {DrumParam::forPad(7, DrumParam::decay), "P8 Decay", 0.01, 4.0, 0.45, false},
    {DrumParam::forPad(7, DrumParam::tone), "P8 Tone", 0.0, 1.0, 0.8, false},
    {DrumParam::forPad(7, DrumParam::level), "P8 Level", 0.0, 1.0, 0.55, false},
    {DrumParam::forPad(7, DrumParam::pan), "P8 Pan", -1.0, 1.0, 0.1, false},
    {DrumParam::forPad(7, DrumParam::chokeGroup), "P8 Choke", 0.0, 8.0, 1.0, true},
    {DrumParam::forPad(7, DrumParam::snap), "P8 Snap", 0.0, 1.0, 0.3, false},

    // ── Pad 9 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(8, DrumParam::engine), "P9 Engine", 0.0, 5.0, 4.0, true},
    {DrumParam::forPad(8, DrumParam::tune), "P9 Tune", -24.0, 24.0, 7.0, false},
    {DrumParam::forPad(8, DrumParam::decay), "P9 Decay", 0.01, 4.0, 0.35, false},
    {DrumParam::forPad(8, DrumParam::tone), "P9 Tone", 0.0, 1.0, 0.4, false},
    {DrumParam::forPad(8, DrumParam::level), "P9 Level", 0.0, 1.0, 0.75, false},
    {DrumParam::forPad(8, DrumParam::pan), "P9 Pan", -1.0, 1.0, 0.3, false},
    {DrumParam::forPad(8, DrumParam::chokeGroup), "P9 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(8, DrumParam::snap), "P9 Snap", 0.0, 1.0, 0.4, false},

    // ── Pad 10 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(9, DrumParam::engine), "P10 Engine", 0.0, 5.0, 4.0, true},
    {DrumParam::forPad(9, DrumParam::tune), "P10 Tune", -24.0, 24.0, 2.0, false},
    {DrumParam::forPad(9, DrumParam::decay), "P10 Decay", 0.01, 4.0, 0.45, false},
    {DrumParam::forPad(9, DrumParam::tone), "P10 Tone", 0.0, 1.0, 0.4, false},
    {DrumParam::forPad(9, DrumParam::level), "P10 Level", 0.0, 1.0, 0.75, false},
    {DrumParam::forPad(9, DrumParam::pan), "P10 Pan", -1.0, 1.0, 0.0, false},
    {DrumParam::forPad(9, DrumParam::chokeGroup), "P10 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(9, DrumParam::snap), "P10 Snap", 0.0, 1.0, 0.4, false},

    // ── Pad 11 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(10, DrumParam::engine), "P11 Engine", 0.0, 5.0, 4.0, true},
    {DrumParam::forPad(10, DrumParam::tune), "P11 Tune", -24.0, 24.0, -5.0, false},
    {DrumParam::forPad(10, DrumParam::decay), "P11 Decay", 0.01, 4.0, 0.6, false},
    {DrumParam::forPad(10, DrumParam::tone), "P11 Tone", 0.0, 1.0, 0.35, false},
    {DrumParam::forPad(10, DrumParam::level), "P11 Level", 0.0, 1.0, 0.75, false},
    {DrumParam::forPad(10, DrumParam::pan), "P11 Pan", -1.0, 1.0, -0.3, false},
    {DrumParam::forPad(10, DrumParam::chokeGroup), "P11 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(10, DrumParam::snap), "P11 Snap", 0.0, 1.0, 0.4, false},

    // ── Pad 12 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(11, DrumParam::engine), "P12 Engine", 0.0, 5.0, 2.0, true},
    {DrumParam::forPad(11, DrumParam::tune), "P12 Tune", -24.0, 24.0, 7.0, false},
    {DrumParam::forPad(11, DrumParam::decay), "P12 Decay", 0.01, 4.0, 0.18, false},
    {DrumParam::forPad(11, DrumParam::tone), "P12 Tone", 0.0, 1.0, 0.9, false},
    {DrumParam::forPad(11, DrumParam::level), "P12 Level", 0.0, 1.0, 0.5, false},
    {DrumParam::forPad(11, DrumParam::pan), "P12 Pan", -1.0, 1.0, 0.35, false},
    {DrumParam::forPad(11, DrumParam::chokeGroup), "P12 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(11, DrumParam::snap), "P12 Snap", 0.0, 1.0, 0.3, false},

    // ── Pad 13 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(12, DrumParam::engine), "P13 Engine", 0.0, 5.0, 5.0, true},
    {DrumParam::forPad(12, DrumParam::tune), "P13 Tune", -24.0, 24.0, 9.0, false},
    {DrumParam::forPad(12, DrumParam::decay), "P13 Decay", 0.01, 4.0, 0.07, false},
    {DrumParam::forPad(12, DrumParam::tone), "P13 Tone", 0.0, 1.0, 0.85, false},
    {DrumParam::forPad(12, DrumParam::level), "P13 Level", 0.0, 1.0, 0.5, false},
    {DrumParam::forPad(12, DrumParam::pan), "P13 Pan", -1.0, 1.0, -0.35, false},
    {DrumParam::forPad(12, DrumParam::chokeGroup), "P13 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(12, DrumParam::snap), "P13 Snap", 0.0, 1.0, 0.7, false},

    // ── Pad 14 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(13, DrumParam::engine), "P14 Engine", 0.0, 5.0, 1.0, true},
    {DrumParam::forPad(13, DrumParam::tune), "P14 Tune", -24.0, 24.0, -4.0, false},
    {DrumParam::forPad(13, DrumParam::decay), "P14 Decay", 0.01, 4.0, 0.35, false},
    {DrumParam::forPad(13, DrumParam::tone), "P14 Tone", 0.0, 1.0, 0.35, false},
    {DrumParam::forPad(13, DrumParam::level), "P14 Level", 0.0, 1.0, 0.6, false},
    {DrumParam::forPad(13, DrumParam::pan), "P14 Pan", -1.0, 1.0, -0.1, false},
    {DrumParam::forPad(13, DrumParam::chokeGroup), "P14 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(13, DrumParam::snap), "P14 Snap", 0.0, 1.0, 0.4, false},

    // ── Pad 15 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(14, DrumParam::engine), "P15 Engine", 0.0, 5.0, 3.0, true},
    {DrumParam::forPad(14, DrumParam::tune), "P15 Tune", -24.0, 24.0, 5.0, false},
    {DrumParam::forPad(14, DrumParam::decay), "P15 Decay", 0.01, 4.0, 0.22, false},
    {DrumParam::forPad(14, DrumParam::tone), "P15 Tone", 0.0, 1.0, 0.75, false},
    {DrumParam::forPad(14, DrumParam::level), "P15 Level", 0.0, 1.0, 0.55, false},
    {DrumParam::forPad(14, DrumParam::pan), "P15 Pan", -1.0, 1.0, 0.28, false},
    {DrumParam::forPad(14, DrumParam::chokeGroup), "P15 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(14, DrumParam::snap), "P15 Snap", 0.0, 1.0, 0.6, false},

    // ── Pad 16 ──────────────────────────────────────────────────────────
    {DrumParam::forPad(15, DrumParam::engine), "P16 Engine", 0.0, 5.0, 0.0, true},
    {DrumParam::forPad(15, DrumParam::tune), "P16 Tune", -24.0, 24.0, 5.0, false},
    {DrumParam::forPad(15, DrumParam::decay), "P16 Decay", 0.01, 4.0, 0.3, false},
    {DrumParam::forPad(15, DrumParam::tone), "P16 Tone", 0.0, 1.0, 0.5, false},
    {DrumParam::forPad(15, DrumParam::level), "P16 Level", 0.0, 1.0, 0.7, false},
    {DrumParam::forPad(15, DrumParam::pan), "P16 Pan", -1.0, 1.0, 0.0, false},
    {DrumParam::forPad(15, DrumParam::chokeGroup), "P16 Choke", 0.0, 8.0, 0.0, true},
    {DrumParam::forPad(15, DrumParam::snap), "P16 Snap", 0.0, 1.0, 0.9, false},

};
constexpr std::size_t parameterCount = std::size(parameters);
static_assert(parameterCount == DrumMachine::parameterCount,
              "the value storage and the descriptor table must be the same size");

constexpr std::uint32_t highestId =
    DrumParam::forPad(DrumMachine::padCount - 1, DrumParam::snap);

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

/// xorshift32. A drum machine needs noise on the audio thread, and noise
/// needs a generator that cannot allocate, cannot lock and cannot surprise —
/// three properties std::mt19937 does not offer in a realtime context.
[[nodiscard]] double nextNoise(std::uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    // 32 bits to [-1, 1).
    return static_cast<double>(state) * (2.0 / 4294967296.0) - 1.0;
}

/// Exponential decay to a time constant, as a multiplier at `seconds`.
[[nodiscard]] double decayAt(double seconds, double timeConstant) noexcept
{
    if (timeConstant <= 0.0)
        return 0.0;

    return std::exp(-seconds / timeConstant);
}

constexpr std::size_t voiceSlot(int index) noexcept { return static_cast<std::size_t>(index); }

/// Below this the voice is inaudible and its slot is worth more than its tail.
constexpr double silenceFloor = 1.0e-4;

/// How long it must stay there first. A clap is three bursts with gaps of
/// about ten milliseconds; retiring inside one of those would cut the clap in
/// half.
constexpr int silenceSamples = 2048;

/// How long a choked or retriggered voice takes to get out of the way. Short
/// enough to be a cut, long enough not to be a click.
constexpr double chokeSeconds = 0.004;

} // namespace

const dsp::EffectParameter* drumParameters() noexcept { return parameters; }
std::size_t                 drumParameterCount() noexcept { return parameterCount; }

const char* drumEngineName(DrumMachine::Engine engine) noexcept
{
    switch (engine) {
        case DrumMachine::Engine::kick:  return "Kick";
        case DrumMachine::Engine::snare: return "Snare";
        case DrumMachine::Engine::hat:   return "Hat";
        case DrumMachine::Engine::clap:  return "Clap";
        case DrumMachine::Engine::tom:   return "Tom";
        case DrumMachine::Engine::rim:   return "Rim";
    }

    return "Kick";
}

int DrumMachine::padForKey(int key) noexcept
{
    const int pad = key - firstKey;
    return (pad >= 0 && pad < padCount) ? pad : -1;
}

DrumMachine::DrumMachine()
{
    for (std::size_t index = 0; index < parameterCount; ++index)
        values_[index].store(parameters[index].defaultValue, std::memory_order_relaxed);
}

void DrumMachine::prepare(SampleRate sampleRate, FrameCount maxBlockSize)
{
    (void)maxBlockSize;
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    allNotesOff();
}

void DrumMachine::allNotesOff() noexcept
{
    for (Voice& voice : voices_)
        voice = {};
}

int DrumMachine::activeVoiceCount() const noexcept
{
    int count = 0;
    for (const Voice& voice : voices_)
        if (voice.isActive())
            ++count;

    return count;
}

double DrumMachine::value(std::uint32_t parameterId) const noexcept
{
    const std::size_t slot = slotFor(parameterId);
    if (slot >= parameterCount)
        return 0.0;

    return values_[slot].load(std::memory_order_relaxed);
}

void DrumMachine::setParameter(std::uint32_t parameterId, double plainValue) noexcept
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

DrumMachine::Settings DrumMachine::readSettings() const noexcept
{
    const auto read = [this](std::uint32_t parameterId) {
        return values_[slotFor(parameterId)].load(std::memory_order_relaxed);
    };

    Settings settings;
    settings.gain = read(DrumParam::gain);

    for (int pad = 0; pad < padCount; ++pad) {
        PadSettings& target = settings.pads[static_cast<std::size_t>(pad)];

        target.engine = static_cast<Engine>(std::clamp(
            static_cast<int>(read(DrumParam::forPad(pad, DrumParam::engine))), 0,
            engineCount - 1));
        target.tune       = read(DrumParam::forPad(pad, DrumParam::tune));
        target.decay      = read(DrumParam::forPad(pad, DrumParam::decay));
        target.tone       = read(DrumParam::forPad(pad, DrumParam::tone));
        target.level      = read(DrumParam::forPad(pad, DrumParam::level));
        target.pan        = read(DrumParam::forPad(pad, DrumParam::pan));
        target.chokeGroup =
            static_cast<int>(read(DrumParam::forPad(pad, DrumParam::chokeGroup)));
        target.snap       = read(DrumParam::forPad(pad, DrumParam::snap));
    }

    return settings;
}

int DrumMachine::findVoiceToSteal() const noexcept
{
    for (int index = 0; index < maxVoices; ++index)
        if (!voices_[voiceSlot(index)].isActive())
            return index;

    // Everything is sounding: the oldest hit is the one furthest into its
    // decay, which on percussion is also the quietest.
    std::uint64_t oldest = ~0ull;
    int           best   = 0;

    for (int index = 0; index < maxVoices; ++index)
        if (voices_[voiceSlot(index)].startedAt < oldest) {
            oldest = voices_[voiceSlot(index)].startedAt;
            best   = index;
        }

    return best;
}

void DrumMachine::trigger(int pad, int velocity, const Settings& settings) noexcept
{
    const PadSettings& padSettings = settings.pads[static_cast<std::size_t>(pad)];

    // Retriggering a pad, and choking its group, are the same gesture: the
    // old voice fades rather than stopping dead, because a hard cut on a
    // sounding drum is a click.
    for (Voice& voice : voices_) {
        if (!voice.isActive() || voice.choking)
            continue;

        const bool samePad = voice.pad == pad;
        const bool sameGroup = padSettings.chokeGroup != 0
                            && voice.settings.chokeGroup == padSettings.chokeGroup;

        if (samePad || sameGroup)
            voice.choking = true;
    }

    Voice& voice = voices_[voiceSlot(findVoiceToSteal())];

    voice           = {};
    voice.pad       = pad;
    voice.velocity  = static_cast<double>(velocity) / 127.0;
    voice.startedAt = ++voiceCounter_;
    voice.settings  = padSettings;

    // A different seed per hit, so two pads struck together do not produce
    // correlated noise and sound like one louder pad.
    voice.noise = 0x9E3779B9u * static_cast<std::uint32_t>(voice.startedAt)
                + static_cast<std::uint32_t>(pad) * 0x85EBCA6Bu + 1u;
}

double DrumMachine::renderVoice(Voice& voice) noexcept
{
    const PadSettings& pad = voice.settings;

    const double tuning = std::exp2(pad.tune / 12.0);
    const double decay  = std::max(pad.decay, 0.005);

    double value = 0.0;

    switch (pad.engine) {
        case Engine::kick:
        case Engine::tom: {
            const bool   isKick     = pad.engine == Engine::kick;
            const double baseHz     = (isKick ? 48.0 : 110.0) * tuning;
            const double dropAmount = isKick ? 1.0 + 7.0 * pad.snap : 1.0 + 2.5 * pad.snap;
            const double dropTime   = isKick ? 0.03 + 0.05 * (1.0 - pad.snap) : 0.06;

            const double sweep = 1.0 + (dropAmount - 1.0) * decayAt(voice.age, dropTime);
            voice.phase += baseHz * sweep / sampleRate_;

            const double body = std::sin(2.0 * pi * voice.phase) * decayAt(voice.age, decay);

            // The click is what makes a kick audible on a small speaker.
            const double click = nextNoise(voice.noise) * decayAt(voice.age, 0.0015)
                               * pad.snap * (isKick ? 0.35 : 0.2);

            value = body + click * (0.4 + 0.6 * pad.tone);
            break;
        }

        case Engine::snare: {
            const double bodyHz = 180.0 * tuning;

            voice.phase    += bodyHz / sampleRate_;
            voice.phaseTwo += bodyHz * 1.78 / sampleRate_;   // an inharmonic partner

            const double body = (std::sin(2.0 * pi * voice.phase) * 0.6
                                 + std::sin(2.0 * pi * voice.phaseTwo) * 0.4)
                              * decayAt(voice.age, decay * 0.5);

            // Rattle: noise through a highpass, which is what the SVF is for.
            const double cutoff = 900.0 + 3500.0 * pad.tone;
            const double f = std::min(2.0 * std::sin(pi * cutoff / sampleRate_), 1.0);
            const double input = nextNoise(voice.noise);
            const double high  = input - voice.filter.low - 1.2 * voice.filter.band;
            voice.filter.band += f * high;
            voice.filter.low  += f * voice.filter.band;

            const double rattle = high * decayAt(voice.age, decay);

            value = body * (1.0 - pad.tone * 0.7) + rattle * (0.35 + 0.65 * pad.tone);
            value += nextNoise(voice.noise) * decayAt(voice.age, 0.001) * pad.snap * 0.4;
            break;
        }

        case Engine::hat: {
            const double cutoff = std::min(4000.0 + 6000.0 * pad.tone, sampleRate_ * 0.45)
                                * tuning;
            const double f = std::min(2.0 * std::sin(pi * std::min(cutoff, sampleRate_ * 0.45)
                                                     / sampleRate_),
                                      1.0);

            const double input = nextNoise(voice.noise);
            const double high  = input - voice.filter.low - 0.9 * voice.filter.band;
            voice.filter.band += f * high;
            voice.filter.low  += f * voice.filter.band;

            // A hat's envelope is nearly all attack: the shorter the decay,
            // the more of the transient survives.
            value = high * decayAt(voice.age, decay * (0.3 + 0.7 * pad.snap));
            break;
        }

        case Engine::clap: {
            // Three bursts a few milliseconds apart, then the tail. That
            // spacing is the whole difference between a clap and a snare.
            const double burst = decayAt(voice.age, 0.0025)
                               + (voice.age > 0.010 ? decayAt(voice.age - 0.010, 0.0025) : 0.0)
                               + (voice.age > 0.021 ? decayAt(voice.age - 0.021, 0.0025) : 0.0);

            const double tail = voice.age > 0.030 ? decayAt(voice.age - 0.030, decay) * 0.55
                                                  : 0.0;

            const double cutoff = (900.0 + 1600.0 * pad.tone) * tuning;
            const double f = std::min(2.0 * std::sin(pi * std::min(cutoff, sampleRate_ * 0.45)
                                                     / sampleRate_),
                                      1.0);

            const double input = nextNoise(voice.noise);
            const double high  = input - voice.filter.low - 0.7 * voice.filter.band;
            voice.filter.band += f * high;
            voice.filter.low  += f * voice.filter.band;

            value = voice.filter.band * (burst * (0.4 + 0.6 * pad.snap) + tail);
            break;
        }

        case Engine::rim: {
            const double clickHz = (1400.0 + 900.0 * pad.tone) * tuning;
            voice.phase += clickHz / sampleRate_;

            const double body = std::sin(2.0 * pi * voice.phase)
                              * decayAt(voice.age, decay * 0.5);

            const double cutoff = (2000.0 + 3000.0 * pad.tone) * tuning;
            const double f = std::min(2.0 * std::sin(pi * std::min(cutoff, sampleRate_ * 0.45)
                                                     / sampleRate_),
                                      1.0);

            const double input = nextNoise(voice.noise);
            const double high  = input - voice.filter.low - 0.6 * voice.filter.band;
            voice.filter.band += f * high;
            voice.filter.low  += f * voice.filter.band;

            value = body * 0.6 + voice.filter.band * decayAt(voice.age, decay * 0.3)
                                     * (0.4 + 0.6 * pad.snap);
            break;
        }
    }

    if (voice.phase >= 1.0)
        voice.phase -= std::floor(voice.phase);
    if (voice.phaseTwo >= 1.0)
        voice.phaseTwo -= std::floor(voice.phaseTwo);

    if (voice.choking) {
        voice.fadeOut -= 1.0 / (chokeSeconds * sampleRate_);
        if (voice.fadeOut <= 0.0) {
            voice.fadeOut = 0.0;
            voice.pad     = -1;
        }
    }

    voice.age += 1.0 / sampleRate_;

    const double output = value * voice.velocity * voice.fadeOut;

    if (std::abs(output) < silenceFloor)
        ++voice.quietFor;
    else
        voice.quietFor = 0;

    if (voice.quietFor > silenceSamples)
        voice.pad = -1;

    return output;
}

void DrumMachine::handleMessage(const MidiMessage& message) noexcept
{
    if (message.isNoteOn()) {
        const int pad = padForKey(message.noteNumber());
        if (pad >= 0)
            trigger(pad, message.velocity(), readSettings());

        return;
    }

    // Note-off is deliberately ignored: a drum's length is its decay, not how
    // long the key was held. Choking is what stops one, and that is a pad
    // setting rather than a gesture.

    if (message.isControlChange() && (message.data1 == 123 || message.data1 == 120))
        allNotesOff();
}

void DrumMachine::renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept
{
    if (frameCount <= 0 || output.channelCount() == 0)
        return;

    const Settings settings = readSettings();

    Sample* left  = output.channel(0);
    Sample* right = output.channelCount() > 1 ? output.channel(1) : nullptr;

    for (Voice& voice : voices_) {
        if (!voice.isActive())
            continue;

        // Constant-power pan, computed once per block per voice. A pad's pan
        // is a mix decision, not something that moves inside a hit.
        const double angle = (std::clamp(voice.settings.pan, -1.0, 1.0) + 1.0) * 0.25 * pi;
        const double gainL = std::cos(angle) * voice.settings.level * settings.gain;
        const double gainR = std::sin(angle) * voice.settings.level * settings.gain;

        for (FrameCount frame = 0; frame < frameCount; ++frame) {
            const double value = renderVoice(voice);

            if (right != nullptr) {
                left[frame]  += static_cast<Sample>(value * gainL);
                right[frame] += static_cast<Sample>(value * gainR);
            } else {
                left[frame] += static_cast<Sample>(value * (gainL + gainR) * 0.7071);
            }

            // A hit that has decayed past hearing has already given its slot
            // back inside renderVoice; there is nothing more to add.
            if (!voice.isActive())
                break;
        }
    }

    // Three or more channels get the left-right pair folded down; INCDAW is
    // stereo above the instruments, so this is the odd case, not the normal
    // one.
    for (std::size_t channel = 2; channel < output.channelCount(); ++channel) {
        Sample* destination = output.channel(channel);
        for (FrameCount frame = 0; frame < frameCount; ++frame)
            destination[frame] += (left[frame] + (right != nullptr ? right[frame] : 0.0f)) * 0.5f;
    }
}

} // namespace incdaw::engine
