#pragma once

#include "engine/core/Time.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/transport/Transport.h"

#include <atomic>
#include <cstdint>

namespace incdaw::engine {

class TempoMap;

/// What INCDAW does about MIDI clock.
///
/// One setting rather than two checkboxes, because sending and receiving are
/// mutually exclusive: a machine that both drives the clock and follows it is
/// chasing its own tail.
enum class MidiClockRole : std::uint8_t {
    off,
    send,
};

/// MIDI beat clock, generated from the tempo map.
///
/// The clock is 24 pulses per quarter note, which at INCDAW's 960 ticks per
/// quarter is one pulse every 40 ticks. Each pulse is placed on the frame the
/// tempo map says that tick falls on — not on a running average of the tempo,
/// and not at the start of whichever block noticed it. That distinction is the
/// whole feature: a tempo change written on the timeline is *followed* by the
/// gear rather than approximated by it, and a pulse lands on its own sample
/// the same way a note does.
///
/// Stopped, there is no timeline to derive pulses from, so the clock free-runs
/// at the tempo under the playhead. External gear with an arpeggiator or a
/// delay needs to know the tempo before the transport moves, and a clock that
/// only exists during playback tells it nothing until it is too late.
///
/// Position changes — a seek, a loop wrap — are sent as stop, song position,
/// continue. Song position pointer is only defined while stopped, so the
/// bracket is not politeness; a receiver is entitled to ignore a bare one.
///
/// Realtime-safe: the generator runs on the audio thread, writes into the
/// block's outgoing buffer, and does bounded work (the pulse loop is capped,
/// and the tempo map's lookups are binary searches over a precomputed table).
class MidiClockGenerator {
public:
    /// One pulse every 40 ticks: 960 / 24.
    static constexpr Tick ticksPerPulse = ticksPerQuarterNote / 24;

    /// A song position pointer counts MIDI beats, which are sixteenth notes.
    static constexpr Tick ticksPerSongPositionBeat = ticksPerQuarterNote / 4;

    /// The clock cannot pulse faster than this within one block. At 999 BPM
    /// and 48 kHz a 4096-frame block holds about 136 pulses; the cap is well
    /// clear of that and exists so a corrupt tempo map cannot spin the audio
    /// thread.
    static constexpr int maxPulsesPerBlock = 512;

    void setRole(MidiClockRole role) noexcept { role_.store(role, std::memory_order_relaxed); }
    [[nodiscard]] MidiClockRole role() const noexcept { return role_.load(std::memory_order_relaxed); }

    /// Forgets where the clock was. Called when the device restarts: the
    /// previous run's pulse phase belongs to a clock that is no longer running.
    void reset() noexcept;

    /// Audio thread. Appends this block's clock traffic to `destination`.
    ///
    /// `segments` is the transport's plan for the block, as returned by
    /// Transport::processBlock; `playing` is what that plan was made under.
    /// A block with no segments (the transport produced none) still generates
    /// the free-running clock, so the gear does not stall.
    void generate(MidiBuffer&         destination,
                  const BlockSegment* segments,
                  std::size_t         segmentCount,
                  const TempoMap&     tempoMap,
                  bool                playing,
                  FrameCount          blockSize) noexcept;

    /// Pulses emitted since the last `reset`. Diagnostics and tests.
    [[nodiscard]] std::uint64_t pulseCount() const noexcept { return pulses_.load(std::memory_order_relaxed); }

private:
    void emitPositionBracket(MidiBuffer& destination, Tick tick, FrameCount offset) const noexcept;

    std::atomic<MidiClockRole> role_{MidiClockRole::off};
    std::atomic<std::uint64_t> pulses_{0};

    bool          wasPlaying_   = false;
    bool          hasHistory_   = false;

    /// The timeline frame the next block was expected to start on. A mismatch
    /// is a seek or a loop wrap, and either needs the receiver told.
    FramePosition expectedFrame_ = 0;

    /// Index of the next clock pulse, in pulses from song start. Held rather
    /// than recomputed so a pulse cannot be emitted twice or skipped when a
    /// block boundary falls next to one.
    std::int64_t nextPulse_ = 0;

    /// Frames accumulated towards the next free-running pulse while stopped.
    double stoppedPhaseFrames_ = 0.0;
};

} // namespace incdaw::engine
