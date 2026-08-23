#pragma once

// INCDAW Drum — sixteen pads of synthesised percussion (A4, CLAUDE.md §13).
//
// The gap this fills is pad-shaped rather than sampler-shaped. The Sampler
// already has zones, layering and key ranges, so sixteen SAMPLES across
// sixteen keys is something INCDAW can already do; what it cannot do is give
// each pad its own voice, its own tuning and envelope, and a choke group —
// the things that make a drum instrument rather than a keyboard holding drum
// sounds.
//
// Every voice is synthesised. Nothing is recorded and nothing is bundled
// (§20/§43), and a synthesised kick has a property a sample does not: its
// pitch, its decay and its click are parameters, so they automate.
//
// Pads sit on MIDI keys 36 upward — one octave and a third of the keyboard,
// which is what the Channel Rack's step grid writes into.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/instrument/Instrument.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace incdaw::engine {

/// Parameter ids. `100 + pad*10 + offset`, plus the output gain at 0. Frozen:
/// they key the state blob and every saved preset.
namespace DrumParam {

inline constexpr std::uint32_t gain = 0;

inline constexpr std::uint32_t padBase   = 100;
inline constexpr std::uint32_t padStride = 10;

enum PadOffset : std::uint32_t {
    engine      = 0,   ///< which voice this pad plays
    tune        = 1,   ///< semitones
    decay       = 2,   ///< seconds
    tone        = 3,   ///< 0 dark/body … 1 bright/noise
    level       = 4,
    pan         = 5,   ///< -1 left … +1 right
    chokeGroup  = 6,   ///< 0 none, 1..8 mutually exclusive
    snap        = 7,   ///< transient click and pitch drop
};

[[nodiscard]] constexpr std::uint32_t forPad(int pad, PadOffset offset) noexcept
{
    return padBase + static_cast<std::uint32_t>(pad) * padStride
         + static_cast<std::uint32_t>(offset);
}

} // namespace DrumParam

class DrumMachine final : public Instrument, public ParameterSink {
public:
    static constexpr int padCount = 16;

    /// The lowest key that plays a pad. Pads run upward from here.
    static constexpr int firstKey = 36;

    /// More than the pads, so a roll on one pad does not cut itself off with
    /// a click: a retriggered pad's old voice fades out over a few
    /// milliseconds while the new one starts.
    static constexpr int maxVoices = 24;

    /// 16 pads × 8 controls, plus the output gain.
    static constexpr std::size_t parameterCount = 129;

    /// The voices a pad can play. Original synthesis, all of it.
    enum class Engine : int {
        kick  = 0,
        snare = 1,
        hat   = 2,
        clap  = 3,
        tom   = 4,
        rim   = 5,
    };

    static constexpr int engineCount = 6;

    DrumMachine();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "INCDAW Drum"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }

    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;
    [[nodiscard]] double value(std::uint32_t parameterId) const noexcept;

    /// Which pad a key plays, or -1 for a key outside the range.
    [[nodiscard]] static int padForKey(int key) noexcept;

protected:
    void handleMessage(const MidiMessage& message) noexcept override;
    void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept override;

private:
    /// One pad's settings, read once per block.
    struct PadSettings {
        Engine engine     = Engine::kick;
        double tune       = 0.0;
        double decay      = 0.3;
        double tone       = 0.5;
        double level      = 0.8;
        double pan        = 0.0;
        int    chokeGroup = 0;
        double snap       = 0.5;
    };

    struct Settings {
        std::array<PadSettings, padCount> pads{};
        double gain = 0.7;
    };

    /// A two-pole state-variable filter, one per voice. The engines differ in
    /// how they configure it, not in which filter they use.
    struct Filter {
        double low  = 0.0;
        double band = 0.0;

        void reset() noexcept { low = 0.0; band = 0.0; }
    };

    struct Voice {
        int    pad        = -1;
        double age        = 0.0;   ///< seconds since the hit
        double velocity   = 1.0;
        double fadeOut    = 1.0;   ///< 1 normally; falls to 0 when choked
        bool   choking    = false;
        std::uint64_t startedAt = 0;

        /// Consecutive samples below the silence floor. A drum ends when it
        /// is inaudible, which is a fact about its output rather than about
        /// its decay setting — a clap's gaps are why the window is tens of
        /// milliseconds rather than a handful of samples.
        int quietFor = 0;

        double phase      = 0.0;
        double phaseTwo   = 0.0;
        std::uint32_t noise = 0x1234567u;

        Filter filter;

        PadSettings settings{};    ///< captured at the hit, so a live edit
                                   ///< cannot change a note already sounding

        [[nodiscard]] bool isActive() const noexcept { return pad >= 0; }
    };

    [[nodiscard]] Settings readSettings() const noexcept;

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void trigger(int pad, int velocity, const Settings& settings) noexcept;

    /// One sample of one voice, before level and pan. Advances the voice.
    [[nodiscard]] double renderVoice(Voice& voice) noexcept;

    std::array<Voice, maxVoices> voices_{};
    SampleRate                   sampleRate_   = 48000.0;
    std::uint64_t                voiceCounter_ = 0;

    std::array<std::atomic<double>, parameterCount> values_{};
};

/// The instrument's parameter table, in the descriptors the catalogue, the
/// registry and the panel already understand.
[[nodiscard]] const dsp::EffectParameter* drumParameters() noexcept;
[[nodiscard]] std::size_t drumParameterCount() noexcept;

/// The name of an engine, for menus. Never nullptr.
[[nodiscard]] const char* drumEngineName(DrumMachine::Engine engine) noexcept;

} // namespace incdaw::engine
