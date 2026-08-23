#pragma once

// INCDAW FM — six operators and a modulation matrix (A2, CLAUDE.md §13).
//
// The design decision that matters is the MATRIX. Classic FM synths ship a
// numbered list of fixed algorithms, which is a compression of the same idea:
// each algorithm is one wiring of "which operator modulates which". Storing
// the wiring itself as thirty-six ordinary parameters costs nothing, makes
// every route automatable through the registry like anything else, and lets a
// patch sit between two algorithms — which a numbered list cannot express.
//
// The diagonal of that matrix is feedback: an operator routed into itself.
// There is no separate feedback parameter, because there is no separate
// mechanism.
//
// Cycles in the matrix — a feedback loop, or two operators modulating each
// other — are resolved the way FM hardware has always resolved them: an
// operator reads its modulators' output from the PREVIOUS sample. That makes
// every wiring computable in one pass, in operator order, with no topological
// sort and no illegal patches.
//
// The piano's electric voicing contains an FM tine. It is not reused here and
// must not be: it is voiced for a tine, and belongs to the piano.

#include "engine/dsp/effects/BuiltinEffect.h"
#include "engine/instrument/Instrument.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace incdaw::engine {

/// Parameter ids, laid out so that they can never need renumbering.
///
///   0                      output gain
///   100 + operator*10 + n  the operator's own controls
///   200 + source*8 + dest  how much `source` modulates `dest`
///
/// The ids are frozen: they key the state blob and every saved preset.
namespace FmParam {

inline constexpr std::uint32_t gain = 0;

inline constexpr std::uint32_t operatorBase = 100;
inline constexpr std::uint32_t operatorStride = 10;

enum OperatorOffset : std::uint32_t {
    ratio     = 0,   ///< multiple of the note's frequency
    fixedHz   = 1,   ///< non-zero overrides the ratio: a fixed pitch
    outLevel  = 2,   ///< how much of this operator reaches the output
    attack    = 3,
    decay     = 4,
    sustain   = 5,
    release   = 6,
    detune    = 7,   ///< cents
};

inline constexpr std::uint32_t matrixBase   = 200;
inline constexpr std::uint32_t matrixStride = 8;

[[nodiscard]] constexpr std::uint32_t forOperator(int index, OperatorOffset offset) noexcept
{
    return operatorBase + static_cast<std::uint32_t>(index) * operatorStride
         + static_cast<std::uint32_t>(offset);
}

[[nodiscard]] constexpr std::uint32_t forRoute(int source, int destination) noexcept
{
    return matrixBase + static_cast<std::uint32_t>(source) * matrixStride
         + static_cast<std::uint32_t>(destination);
}

} // namespace FmParam

class FmSynth final : public Instrument, public ParameterSink {
public:
    static constexpr int operatorCount = 6;

    /// Fewer than the wavetable's: an FM voice is six oscillators and six
    /// envelopes, so a voice costs about six times as much.
    static constexpr int maxVoices = 16;

    /// 48 operator controls, 36 routes and the output gain.
    static constexpr std::size_t parameterCount = 85;

    FmSynth();

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "INCDAW FM"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }

    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    [[nodiscard]] double value(std::uint32_t parameterId) const noexcept;

protected:
    void handleMessage(const MidiMessage& message) noexcept override;
    void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept override;

private:
    enum class Stage { idle, attack, decay, sustain, release };

    struct OperatorState {
        double phase    = 0.0;
        double previous = 0.0;   ///< last sample's output, what modulators read
        Stage  stage    = Stage::idle;
        double level    = 0.0;
    };

    struct Voice {
        int           key       = -1;
        int           channel   = 0;
        double        frequency = 0.0;
        double        velocity  = 1.0;
        std::uint64_t startedAt = 0;

        std::array<OperatorState, operatorCount> operators{};

        [[nodiscard]] bool isActive() const noexcept
        {
            for (const OperatorState& state : operators)
                if (state.stage != Stage::idle)
                    return true;

            return false;
        }
    };

    /// One operator's settings, read once per block.
    struct OperatorSettings {
        double increment   = 0.0;   ///< filled per voice
        double ratio       = 1.0;
        double fixedHz     = 0.0;
        double detune      = 0.0;
        double outLevel    = 0.0;
        double attackRate  = 1.0;
        double decayRate   = 1.0;
        double sustain     = 1.0;
        double releaseRate = 1.0;
    };

    struct Settings {
        std::array<OperatorSettings, operatorCount> operators{};
        std::array<double, operatorCount * operatorCount> matrix{};
        double gain = 0.5;
        bool   anyOutput = false;
    };

    [[nodiscard]] Settings readSettings() const noexcept;

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void startVoice(int index, int channel, int key, int velocity) noexcept;
    void releaseVoicesForKey(int channel, int key) noexcept;

    std::array<Voice, maxVoices> voices_{};
    SampleRate                   sampleRate_   = 48000.0;
    std::uint64_t                voiceCounter_ = 0;

    std::array<std::atomic<double>, parameterCount> values_{};
};

/// The synth's parameter table, in the same descriptors a builtin effect
/// publishes — so the catalogue, the registry and the panel need no new shape.
[[nodiscard]] const dsp::EffectParameter* fmParameters() noexcept;
[[nodiscard]] std::size_t fmParameterCount() noexcept;

} // namespace incdaw::engine
