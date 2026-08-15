#pragma once

#include "engine/audio/WavFile.h"
#include "engine/instrument/Instrument.h"

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
    std::shared_ptr<const AudioFileData> sample;

    int rootKey      = 60;    ///< the key at which the sample plays unshifted
    int keyLow       = 0;
    int keyHigh      = 127;
    int velocityLow  = 1;
    int velocityHigh = 127;

    /// The slice that plays, in source frames. end == 0 means "to the end".
    FrameCount start = 0;
    FrameCount end   = 0;

    bool   reverse = false;
    double gain    = 1.0;

    [[nodiscard]] bool matches(int key, int velocity) const noexcept
    {
        return key >= keyLow && key <= keyHigh && velocity >= velocityLow
            && velocity <= velocityHigh;
    }
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
class Sampler final : public Instrument {
public:
    static constexpr int maxVoices = 64;

    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;
    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "Sampler"; }
    [[nodiscard]] int activeVoiceCount() const noexcept override;

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

        [[nodiscard]] bool isActive() const noexcept { return stage != Stage::idle; }
    };

    [[nodiscard]] int findVoiceToSteal() const noexcept;
    void startVoice(int index, const SamplerZone& zone, int channel, int key,
                    int velocity) noexcept;
    void releaseVoicesForKey(int channel, int key) noexcept;
    void renderVoice(Voice& voice, const AudioBufferView& output,
                     FrameCount frameCount) noexcept;

    std::vector<SamplerZone>     zones_;
    std::array<Voice, maxVoices> voices_{};
    SampleRate                   sampleRate_   = 48000.0;
    std::uint64_t                voiceCounter_ = 0;

    std::atomic<double> attack_{0.002};
    std::atomic<double> decay_{0.050};
    std::atomic<double> sustain_{1.0};
    std::atomic<double> release_{0.050};
};

} // namespace incdaw::engine
