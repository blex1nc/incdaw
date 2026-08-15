#pragma once

#include "engine/audio/WavFile.h"
#include "engine/instrument/Instrument.h"
#include "engine/instrument/SamplerStream.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace incdaw::engine {

/// One mapped sample: which keys and velocities it answers, where it sits in
/// pitch, and which slice of the source plays.
///
/// Zones are the whole of multisampling: a program is nothing but zones, and
/// key/velocity LAYERING falls out of several zones matching the same note.
/// The audio is shared and immutable — two zones (or two samplers) mapping
/// the same file hold the same decoded copy.
struct SamplerZone {
    /// The decoded audio: the whole file for a resident zone, or the
    /// preloaded HEAD when `stream` is set.
    std::shared_ptr<const AudioFileData> sample;

    /// Set when the zone streams from disk past the head. Streamed zones are
    /// forward-only and never sustain-loop — a loop has to be resident to be
    /// seamless, so the compiler preloads looped and reversed zones whole.
    std::shared_ptr<SamplerZoneStream> stream;

    int rootKey      = 60;    ///< the key at which the sample plays unshifted
    int keyLow       = 0;
    int keyHigh      = 127;
    int velocityLow  = 1;
    int velocityHigh = 127;

    /// The slice that plays, in source frames. end == 0 means "to the end".
    FrameCount start = 0;
    FrameCount end   = 0;

    /// Sustain loop, in source frames within the slice. loopEnd == 0 means
    /// no loop; a loop that does not fit the slice is ignored rather than
    /// clamped into something the user did not draw. A held note cycles
    /// loopStart..loopEnd forever, through release too — that is what a
    /// sampler's sustain loop means. Reverse zones do not loop (yet).
    FrameCount loopStart = 0;
    FrameCount loopEnd   = 0;

    /// Crossfade at the loop seam, in source frames: the last N frames of
    /// the loop blend into the material just before loopStart, which is what
    /// removes the click a hard seam makes. Clamped to what fits — the loop
    /// length, and the material available before loopStart.
    FrameCount loopCrossfade = 0;

    bool   reverse = false;
    double gain    = 1.0;

    [[nodiscard]] bool matches(int key, int velocity) const noexcept
    {
        return key >= keyLow && key <= keyHigh && velocity >= velocityLow
            && velocity <= velocityHigh;
    }
};

/// The sampler's automatable parameters, in plain units. The table is what
/// the registry registers and the sink switches on; ids are stable, values
/// land on the same atomic setters the UI uses.
enum class SamplerParam : std::uint32_t {
    attackSeconds  = 0,
    decaySeconds   = 1,
    sustainLevel   = 2,
    releaseSeconds = 3,
    filterMode     = 4,
    filterCutoffHz = 5,
    filterResonance = 6,
    lfoRateHz      = 7,
    lfoToPitch     = 8,
    lfoToCutoff    = 9,
};

/// The sampler: zones in, sample-accurate voices out (CLAUDE.md §18).
///
/// Playback repitches by RATE — a note a fifth above the root reads the
/// source ~1.5× faster through linear interpolation. That is what a sampler
/// is; it is not time stretching (§16), which preserves duration and lives in
/// its own subsystem. A note that matches several zones starts several
/// voices, which is velocity layering.
///
/// `setZones` follows the same contract as every structural edit in INCDAW:
/// it happens at BUILD time, off the audio thread, before the instrument
/// renders — a zone edit is a graph rebuild, exactly like an insert edit.
/// Live, realtime-safe control is the envelope setters, like the reference
/// synth. Envelope shape and voice stealing mirror SimpleSynth, because
/// those behaviours are conventions users rely on.
class Sampler final : public Instrument, public ParameterSink {
public:
    static constexpr int maxVoices = 64;

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Sampler"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }

    /// Routes a plain value onto the matching setter. Realtime-safe.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    /// Replaces the program and silences every voice. Build time only: must
    /// not be called while the instrument is rendering (a zone edit is a
    /// graph rebuild, and the compiler constructs instruments fresh).
    void setZones(std::vector<SamplerZone> zones);

    [[nodiscard]] std::size_t zoneCount() const noexcept { return zones_.size(); }

    // ── Envelope, shared by every zone for now. Realtime-safe setters.
    void setAttackSeconds(double seconds) noexcept  { attack_.store(seconds, std::memory_order_relaxed); }
    void setDecaySeconds(double seconds) noexcept   { decay_.store(seconds, std::memory_order_relaxed); }
    void setSustainLevel(double level) noexcept     { sustain_.store(level, std::memory_order_relaxed); }
    void setReleaseSeconds(double seconds) noexcept { release_.store(seconds, std::memory_order_relaxed); }

    // ── Filter and LFO, shared by every zone like the envelope. The filter
    // is per VOICE (each note filters its own signal, the polyphonic
    // convention); these set the shared parameters. Realtime-safe setters.
    enum class FilterMode : int { off = 0, lowpass = 1, highpass = 2, bandpass = 3 };

    void setFilterMode(FilterMode mode) noexcept
    {
        filterMode_.store(static_cast<int>(mode), std::memory_order_relaxed);
    }
    void setFilterCutoffHz(double hz) noexcept  { filterCutoff_.store(hz, std::memory_order_relaxed); }
    void setFilterResonance(double q) noexcept  { filterResonance_.store(q, std::memory_order_relaxed); }

    [[nodiscard]] double filterCutoffHz() const noexcept
    {
        return filterCutoff_.load(std::memory_order_relaxed);
    }

    /// One LFO, retriggered per note, sine. Destinations are depth-controlled:
    /// zero depth is off, so there is no separate enable.
    void setLfoRateHz(double hz) noexcept             { lfoRate_.store(hz, std::memory_order_relaxed); }
    void setLfoToPitchSemitones(double semis) noexcept { lfoToPitch_.store(semis, std::memory_order_relaxed); }
    void setLfoToCutoffOctaves(double octaves) noexcept { lfoToCutoff_.store(octaves, std::memory_order_relaxed); }

protected:
    void handleMessage(const MidiMessage& message) noexcept override;
    void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept override;

private:
    enum class Stage { idle, attack, decay, sustain, release };

    struct Voice {
        Stage stage   = Stage::idle;
        int   key     = -1;
        int   channel = 0;

        const SamplerZone* zone = nullptr;   ///< into zones_, stable until setZones

        double        position  = 0.0;   ///< source frames, fractional
        double        rate      = 0.0;   ///< source frames per output frame
        double        level     = 0.0;   ///< envelope output
        double        velocity  = 1.0;
        std::uint64_t startedAt = 0;

        /// State-variable filter state, one per output channel up to a fixed
        /// count — voices are preallocated, so this cannot grow at render time.
        static constexpr std::size_t maxFilterChannels = 8;

        struct FilterState {
            double low  = 0.0;
            double band = 0.0;
        };

        std::array<FilterState, maxFilterChannels> filter{};

        double lfoPhase = 0.0;   ///< cycles, 0..1; retriggered per note

        /// Streaming: the claimed pool slot, or -1 (head-only voice). The
        /// owner pointer is what release needs after zones_ has changed.
        int                streamSlot  = -1;
        SamplerZoneStream* streamOwner = nullptr;

        /// Per-block fetch of the streamed span, one buffer per source
        /// channel. Sized in prepare; empty until a streamed zone exists.
        std::array<std::vector<Sample>, SamplerZoneStream::maxSourceChannels> scratch;

        [[nodiscard]] bool isActive() const noexcept { return stage != Stage::idle; }
    };

    /// The highest playback rate a streamed voice may reach (root shift and
    /// LFO combined): the per-block fetch is sized by it. Three octaves up.
    static constexpr double maxStreamRate = 8.0;

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void startVoice(int index, const SamplerZone& zone, int channel, int key,
                    int velocity) noexcept;
    void releaseVoicesForKey(int channel, int key) noexcept;
    void renderVoice(Voice& voice, const AudioBufferView& output,
                     FrameCount frameCount) noexcept;

    /// Ends a voice, returning its stream slot to the pool.
    void endVoice(Voice& voice) noexcept;

    /// Allocates the per-voice fetch buffers once streaming is in play.
    /// Off the audio thread by contract, like prepare and setZones.
    void ensureStreamScratch();

    std::vector<SamplerZone>     zones_;
    std::array<Voice, maxVoices> voices_{};
    SampleRate                   sampleRate_   = 48000.0;
    FrameCount                   maxBlockSize_ = 0;
    std::uint64_t                voiceCounter_ = 0;

    std::atomic<double> attack_{0.002};
    std::atomic<double> decay_{0.050};
    std::atomic<double> sustain_{1.0};
    std::atomic<double> release_{0.050};

    std::atomic<int>    filterMode_{0};
    std::atomic<double> filterCutoff_{20000.0};
    std::atomic<double> filterResonance_{0.7071};
    std::atomic<double> lfoRate_{5.0};
    std::atomic<double> lfoToPitch_{0.0};
    std::atomic<double> lfoToCutoff_{0.0};
};

} // namespace incdaw::engine
