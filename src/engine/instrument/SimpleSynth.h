#pragma once

#include "engine/instrument/Instrument.h"

#include <array>
#include <atomic>

namespace incdaw::engine {

/// The reference instrument: a polyphonic subtractive voice.
///
/// Its purpose is to prove the instrument API and make the application audible,
/// so it is deliberately small — but it is not a placeholder. CLAUDE.md §17
/// forbids shipping mocked DSP dressed as production, so the parts that are
/// here are done properly:
///
///   - the oscillators are band-limited with PolyBLEP. A naive sawtooth is four
///     lines shorter and aliases audibly on every note above the middle of the
///     keyboard, which is exactly the sort of "looks correct" DSP the
///     constitution rules out.
///   - the envelope is a real ADSR with a release stage, so voices are stolen
///     from the quietest release rather than cut off mid-note.
///   - voice stealing prefers released voices, then the oldest.
/// The reference synth's automatable parameters, in plain units.
enum class SimpleSynthParam : std::uint32_t {
    waveform       = 0,
    gain           = 1,
    attackSeconds  = 2,
    decaySeconds   = 3,
    sustainLevel   = 4,
    releaseSeconds = 5,
};

class SimpleSynth final : public Instrument, public ParameterSink {
public:
    static constexpr int maxVoices = 32;

    enum class Waveform { sine, sawtooth, square, triangle };

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Reference Synth"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }

    /// Routes a plain value onto the matching setter. Realtime-safe.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    // ── Parameters. Realtime-safe: written by the UI, read by the audio thread.
    void setWaveform(Waveform waveform) noexcept { waveform_.store(waveform, std::memory_order_relaxed); }
    void setGain(Sample gain) noexcept { gain_.store(gain, std::memory_order_relaxed); }

    void setAttackSeconds(double seconds) noexcept  { attack_.store(seconds, std::memory_order_relaxed); }
    void setDecaySeconds(double seconds) noexcept   { decay_.store(seconds, std::memory_order_relaxed); }
    void setSustainLevel(double level) noexcept     { sustain_.store(level, std::memory_order_relaxed); }
    void setReleaseSeconds(double seconds) noexcept { release_.store(seconds, std::memory_order_relaxed); }

protected:
    void handleMessage(const MidiMessage& message) noexcept override;
    void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept override;

private:
    enum class Stage { idle, attack, decay, sustain, release };

    struct Voice {
        Stage         stage      = Stage::idle;
        int           key        = -1;
        int           channel    = 0;
        double        phase      = 0.0;   ///< cycles, [0,1)
        double        increment  = 0.0;
        double        level      = 0.0;   ///< envelope output
        double        velocity   = 1.0;
        std::uint64_t startedAt  = 0;     ///< for oldest-voice stealing

        [[nodiscard]] bool isActive() const noexcept { return stage != Stage::idle; }
    };

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void startVoice(int index, int channel, int key, int velocity) noexcept;
    void releaseVoicesForKey(int channel, int key) noexcept;

    [[nodiscard]] double sampleWaveform(const Voice& voice, Waveform waveform) const noexcept;

    std::array<Voice, maxVoices> voices_{};
    SampleRate                   sampleRate_ = 48000.0;
    std::uint64_t                voiceCounter_ = 0;

    std::atomic<Waveform> waveform_{Waveform::sawtooth};
    std::atomic<Sample>   gain_{0.22f};

    std::atomic<double> attack_{0.005};
    std::atomic<double> decay_{0.120};
    std::atomic<double> sustain_{0.65};
    std::atomic<double> release_{0.180};
};

} // namespace incdaw::engine
