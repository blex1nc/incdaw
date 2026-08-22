#pragma once

#include "engine/instrument/Instrument.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace incdaw::engine {

/// The piano's automatable parameters, in plain units.
///
/// Ids are stable and are what the project file stores through
/// ChannelInstrumentParameter; never renumber them.
enum class PianoParam : std::uint32_t {
    model        = 0,   ///< PianoModel, as a discrete index
    tone         = 1,   ///< -1 dark … +1 bright
    hardness     = 2,   ///< how much velocity opens the spectrum
    decay        = 3,   ///< multiplier on the natural string decay
    release      = 4,   ///< multiplier on the damper's fall time
    stretchCents = 5,   ///< octave stretch at the extremes, in cents
    hammerNoise  = 6,   ///< the thump of the hammer, 0..1
    pedalTail    = 7,   ///< extra tail while the sustain pedal is down
    stereoSpread = 8,   ///< keyboard spread across the stereo field
    gain         = 9,
};

/// The voicings the instrument can be built as.
///
/// A "model" is not a preset of the parameters above — it changes the physics
/// the voice is built from (inharmonicity, strike point, decay law, and for
/// the electric it swaps the string model for an FM tine). Parameters shape
/// what the model produces; they cannot turn one into another.
enum class PianoModel : int {
    grand       = 0,
    brightGrand = 1,
    upright     = 2,
    mellow      = 3,
    electric    = 4,
};

inline constexpr int pianoModelCount = 5;

/// The name of a model, for menus. Never nullptr — an out-of-range index
/// answers with the grand, which is what an out-of-range model plays.
[[nodiscard]] const char* pianoModelName(int model) noexcept;

/// INCDAW Piano: a synthesized piano (CLAUDE.md §13, §20).
///
/// It is synthesis, not sampling, and that is a deliberate constraint rather
/// than a shortcut: §20 and §43 forbid bundling recorded piano content, so a
/// sample-based piano would ship with nothing to load. What is here is a
/// physical *model* of a struck string, not a sine with an envelope:
///
///   - partials are INHARMONIC. A real string is stiff, so its nth partial
///     sits at n·f0·√(1+Bn²) rather than at n·f0. That stretch is the single
///     most recognisable thing about a piano's timbre; without it the tone is
///     an organ.
///   - the strike point comb is modelled. A hammer hitting one eighth of the
///     way along the string cannot excite the 8th, 16th … partials, which is
///     why a piano's spectrum has holes in it.
///   - each partial decays at its OWN rate, faster the higher it sits, in two
///     stages. Double decay (a fast initial fall into a long tail) is what a
///     coupled string trio does and what a single exponential cannot fake.
///   - the damper is a separate, register-dependent fall applied on note-off;
///     bass strings take far longer to silence than treble ones, and the
///     sustain pedal lifts the dampers entirely (CC 64).
///
/// It is an approximation, and the honest limits are: no true sympathetic
/// resonance across strings, no soft-pedal (una corda) timbre change, and a
/// partial budget that truncates the very lowest notes' spectra.
///
/// Realtime contract: prepare() allocates nothing (every buffer is a fixed
/// member array) and the render path allocates nothing, locks nothing and
/// calls no transcendental function per sample — partial frequencies and
/// decay coefficients are computed once, at note-on.
class PianoInstrument final : public Instrument, public ParameterSink {
public:
    /// A piano is played with the pedal down, so voices accumulate: 64 is what
    /// a two-handed passage with the damper lifted actually needs.
    static constexpr int maxVoices = 64;

    /// Partials per voice. The bass has more than this in reality; the budget
    /// is what keeps 64 voices affordable, and the truncation is documented
    /// rather than hidden.
    static constexpr int maxPartials = 24;

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "INCDAW Piano"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }

    /// Routes a plain value onto the matching setter. Realtime-safe.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    // ── Parameters. Written by the UI, read by the audio thread ─────────────
    void setModel(PianoModel model) noexcept { model_.store(model, std::memory_order_relaxed); }
    void setTone(double tone) noexcept { tone_.store(tone, std::memory_order_relaxed); }
    void setHardness(double hardness) noexcept { hardness_.store(hardness, std::memory_order_relaxed); }
    void setDecay(double decay) noexcept { decay_.store(decay, std::memory_order_relaxed); }
    void setRelease(double release) noexcept { release_.store(release, std::memory_order_relaxed); }
    void setStretchCents(double cents) noexcept { stretch_.store(cents, std::memory_order_relaxed); }
    void setHammerNoise(double amount) noexcept { hammer_.store(amount, std::memory_order_relaxed); }
    void setPedalTail(double amount) noexcept { pedalTail_.store(amount, std::memory_order_relaxed); }
    void setStereoSpread(double amount) noexcept { spread_.store(amount, std::memory_order_relaxed); }
    void setGain(double gain) noexcept { gain_.store(gain, std::memory_order_relaxed); }

    /// True while CC 64 is held. Exposed for tests: a note-off with the pedal
    /// down must not damp, and that is otherwise only observable by listening.
    [[nodiscard]] bool sustainPedalDown() const noexcept { return pedalDown_; }

protected:
    void handleMessage(const MidiMessage& message) noexcept override;
    void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept override;

private:
    /// One partial of one string: a rotating unit vector (two multiplies and
    /// an add per sample, no sin() in the render loop) scaled by two decaying
    /// envelopes.
    struct Partial {
        double cosine = 1.0;     ///< rotation state, real part
        double sine   = 0.0;     ///< rotation state, imaginary part
        double stepCos = 1.0;    ///< per-sample rotation
        double stepSin = 0.0;

        double fastLevel = 0.0;  ///< the initial fall
        double slowLevel = 0.0;  ///< the long tail
        double fastCoef  = 0.0;  ///< per-sample multiplier for each
        double slowCoef  = 0.0;
    };

    struct Voice {
        bool   active   = false;
        bool   held     = false;   ///< key still down (or held by the pedal)
        int    key      = -1;
        int    channel  = 0;
        double velocity = 1.0;

        double leftGain  = 0.7071;
        double rightGain = 0.7071;

        /// The damper's fall, applied over everything once the key is up.
        double damper     = 1.0;
        double damperCoef = 1.0;

        /// Hammer thump: a short filtered noise burst, decaying fast.
        double noiseLevel = 0.0;
        double noiseCoef  = 0.0;
        double noiseState = 0.0;
        std::uint32_t noiseSeed = 1u;

        /// Electric model only: an FM tine. `fmIndex` falls to zero, which is
        /// the bark at the front of a Rhodes note.
        bool   fm            = false;
        double fmCarrierPhase = 0.0;
        double fmModPhase     = 0.0;
        double fmCarrierStep  = 0.0;
        double fmModStep      = 0.0;
        double fmIndex        = 0.0;
        double fmIndexCoef    = 0.0;
        double fmLevel        = 0.0;
        double fmCoef         = 0.0;

        int           partialCount = 0;
        std::uint64_t startedAt    = 0;

        std::array<Partial, maxPartials> partials{};
    };

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void startVoice(int index, int channel, int key, int velocity) noexcept;
    void releaseKey(int channel, int key) noexcept;
    void damp(Voice& voice, double seconds) noexcept;
    [[nodiscard]] double voicePeak(const Voice& voice) const noexcept;

    SampleRate sampleRate_ = 48000.0;

    std::array<Voice, maxVoices> voices_{};
    std::uint64_t                counter_  = 0;
    bool                         pedalDown_ = false;

    std::atomic<PianoModel> model_    {PianoModel::grand};
    std::atomic<double>     tone_     {0.0};
    std::atomic<double>     hardness_ {0.5};
    std::atomic<double>     decay_    {1.0};
    std::atomic<double>     release_  {1.0};
    std::atomic<double>     stretch_  {12.0};
    std::atomic<double>     hammer_   {0.35};
    std::atomic<double>     pedalTail_{0.3};
    std::atomic<double>     spread_   {0.35};
    std::atomic<double>     gain_     {0.7};
};

} // namespace incdaw::engine
