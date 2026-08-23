#pragma once

// INCDAW Wavetable — the first synthesis instrument beyond the reference
// synth (A1, CLAUDE.md §13).
//
// Two wavetable oscillators, a sub, a state-variable filter, two envelopes
// and two LFOs. The modulation shape is the Sampler's: an explicit depth per
// destination rather than a routing matrix, so a lane, a MIDI knob and the
// panel all reach a depth as an ordinary automatable parameter with no
// matrix-shaped special case anywhere.
//
// The oscillators do not alias, which is the whole reason engine/instrument/
// Wavetable.h exists — see its header for why that is the hard part.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/instrument/Instrument.h"
#include "engine/instrument/Wavetable.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace incdaw::engine {

/// The synth's automatable parameters, in plain units.
///
/// The numbering is deliberately sparse and grouped, and it is FROZEN: these
/// ids key the state blob and every saved preset, so a parameter added to the
/// filter block later takes the next free number in that block rather than
/// renumbering what is already on somebody's disk.
enum class WavetableParam : std::uint32_t {
    // ── Oscillator A ─────────────────────────────────────────────────────
    oscATable      = 0,    ///< index into the table catalogue, stepped
    oscAPosition   = 1,    ///< 0..1 across the table's frames
    oscALevel      = 2,
    oscADetune     = 3,    ///< cents
    oscASemitones  = 4,    ///< stepped

    // ── Oscillator B ─────────────────────────────────────────────────────
    oscBTable      = 10,
    oscBPosition   = 11,
    oscBLevel      = 12,
    oscBDetune     = 13,
    oscBSemitones  = 14,

    // ── Sub ──────────────────────────────────────────────────────────────
    subLevel       = 20,
    subOctave      = 21,   ///< -1 or -2, stepped
    subWave        = 22,   ///< 0 sine · 1 square, stepped

    // ── Filter ───────────────────────────────────────────────────────────
    filterMode     = 30,   ///< 0 off · 1 lowpass · 2 highpass · 3 bandpass
    filterCutoffHz = 31,
    filterResonance = 32,
    filterKeytrack = 33,   ///< 0 none … 1 follows the keyboard exactly

    // ── Amplitude envelope ───────────────────────────────────────────────
    ampAttack      = 40,
    ampDecay       = 41,
    ampSustain     = 42,
    ampRelease     = 43,

    // ── Modulation envelope, and where it goes ───────────────────────────
    modAttack      = 50,
    modDecay       = 51,
    modSustain     = 52,
    modRelease     = 53,
    modToCutoff    = 54,   ///< octaves
    modToPosition  = 55,   ///< -1..1 of table position
    modToPitch     = 56,   ///< semitones

    // ── LFO 1 ────────────────────────────────────────────────────────────
    lfo1Shape      = 60,   ///< 0 sine · 1 triangle · 2 square · 3 saw, stepped
    lfo1RateHz     = 61,
    lfo1ToPitch    = 62,   ///< semitones
    lfo1ToCutoff   = 63,   ///< octaves
    lfo1ToPosition = 64,

    // ── LFO 2 ────────────────────────────────────────────────────────────
    lfo2Shape      = 70,
    lfo2RateHz     = 71,
    lfo2ToPitch    = 72,
    lfo2ToCutoff   = 73,
    lfo2ToPosition = 74,

    // ── Output ───────────────────────────────────────────────────────────
    gain           = 80,
};

class WavetableSynth final : public Instrument, public ParameterSink {
public:
    static constexpr int maxVoices = 24;

    /// Size of the parameter table. Declared here because the value storage
    /// is a member; the definition in the .cpp asserts the two agree, so a
    /// parameter added without widening this fails to compile rather than
    /// writing past the end of the array.
    static constexpr std::size_t parameterCount = 39;

    /// Modulation is recomputed every this many frames rather than every
    /// frame. An LFO at 20 Hz moves 0.013 of a cycle in 32 samples at 48 kHz,
    /// which is inaudible, and it keeps the two exponentials a pitch and a
    /// cutoff need out of the per-sample path.
    static constexpr FrameCount controlBlock = 32;

    enum class FilterMode : int { off = 0, lowpass = 1, highpass = 2, bandpass = 3 };
    enum class LfoShape : int { sine = 0, triangle = 1, square = 2, sawtooth = 3 };

    WavetableSynth();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "INCDAW Wavetable"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }

    /// Clamps into the parameter's declared range and stores. Realtime-safe:
    /// a store is the whole hand-off, exactly as a builtin effect's is.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    /// Current plain value of `parameterId`, or its default for an id this
    /// synth does not have. For tests and for state capture.
    [[nodiscard]] double value(std::uint32_t parameterId) const noexcept;

protected:
    void handleMessage(const MidiMessage& message) noexcept override;
    void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept override;

private:
    enum class Stage { idle, attack, decay, sustain, release };

    struct Envelope {
        Stage  stage = Stage::idle;
        double level = 0.0;
    };

    struct Voice {
        int           key       = -1;
        int           channel   = 0;
        double        frequency = 0.0;
        double        velocity  = 1.0;
        std::uint64_t startedAt = 0;

        Envelope amplitude;
        Envelope modulation;

        double phaseA   = 0.0;
        double phaseB   = 0.0;
        double phaseSub = 0.0;

        /// Chosen once at note-on, with headroom for the pitch modulation the
        /// patch can apply — re-picking per sample would cost an exp2 in the
        /// inner loop to fix an audible problem that does not exist.
        std::size_t levelA   = 0;
        std::size_t levelB   = 0;
        std::size_t levelSub = 0;

        double filterLow  = 0.0;
        double filterBand = 0.0;

        [[nodiscard]] bool isActive() const noexcept
        {
            return amplitude.stage != Stage::idle;
        }
    };

    /// Everything the render loop needs, read once per block. Reading 37
    /// atomics inside the voice loop would be 37 × voices × blocks of pure
    /// waste, and would also let a patch change halfway down the voice list.
    struct Settings {
        const Wavetable* tableA = nullptr;
        const Wavetable* tableB = nullptr;
        const Wavetable* tableSub = nullptr;

        double positionA = 0.0, positionB = 0.0, subPosition = 0.0;
        double levelA = 0.0, levelB = 0.0, levelSub = 0.0;
        double tuneA = 0.0, tuneB = 0.0;   ///< semitones, detune folded in
        double subOctave = -1.0;

        FilterMode filterMode = FilterMode::off;
        double     cutoffHz = 20000.0, resonance = 0.7071, keytrack = 0.0;

        double ampAttack = 0.0, ampDecay = 0.0, ampSustain = 1.0, ampRelease = 0.0;
        double modAttack = 0.0, modDecay = 0.0, modSustain = 1.0, modRelease = 0.0;

        double modToCutoff = 0.0, modToPosition = 0.0, modToPitch = 0.0;

        LfoShape lfo1Shape = LfoShape::sine, lfo2Shape = LfoShape::sine;
        double   lfo1Rate = 0.0, lfo2Rate = 0.0;
        double   lfo1ToPitch = 0.0, lfo1ToCutoff = 0.0, lfo1ToPosition = 0.0;
        double   lfo2ToPitch = 0.0, lfo2ToCutoff = 0.0, lfo2ToPosition = 0.0;

        double gain = 0.5;
    };

    [[nodiscard]] Settings readSettings() const noexcept;

    /// The widest pitch excursion the current patch can apply, in semitones —
    /// the headroom the mip choice needs.
    [[nodiscard]] double pitchHeadroom(const Settings& settings) const noexcept;

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void startVoice(int index, int channel, int key, int velocity) noexcept;
    void releaseVoicesForKey(int channel, int key) noexcept;

    static void advanceEnvelope(Envelope& envelope, double attackRate, double decayRate,
                                double sustain, double releaseRate) noexcept;

    std::array<Voice, maxVoices> voices_{};
    SampleRate                   sampleRate_   = 48000.0;
    std::uint64_t                voiceCounter_ = 0;

    double lfo1Phase_ = 0.0;
    double lfo2Phase_ = 0.0;

    /// Plain values, by table index. One atomic per parameter, exactly as a
    /// builtin effect keeps them.
    std::array<std::atomic<double>, parameterCount> values_{};
};

/// The synth's parameter table — the same descriptors a builtin effect
/// publishes, so the catalogue, the registry and the panel need no new shape.
[[nodiscard]] const dsp::EffectParameter* wavetableParameters() noexcept;
[[nodiscard]] std::size_t wavetableParameterCount() noexcept;

} // namespace incdaw::engine
