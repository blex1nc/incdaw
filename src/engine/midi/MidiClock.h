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
    receive,
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

/// The other direction: INCDAW's transport following someone else's clock.
///
/// What this does and does not claim matters, so it is written down rather
/// than implied. Start, stop, continue and song position are followed exactly
/// — the external machine's transport is INCDAW's transport, and a locate on
/// the master is a locate here, on the frame the message arrived on. The
/// *tempo* is measured from the pulse intervals and published; applying it to
/// the project is the shell's job, not the audio thread's, because installing
/// a tempo map allocates.
///
/// What it does not do is phase-lock: between locates, INCDAW's timeline
/// advances at the project's own tempo rather than being nudged pulse by
/// pulse. A master running at a slightly different tempo therefore drifts
/// until the next locate. Closing that loop needs the timeline to run at a
/// rate the audio path can follow, which is a larger change than this one.
///
/// The jitter filter is the reason a measured tempo is worth having at all.
/// Clock arrives over a bus with a millisecond of slop in it; feeding raw
/// intervals to anything produces a tempo that visibly shakes. Intervals far
/// from the running estimate are rejected as slop rather than averaged in —
/// and a run of consecutive rejections is read as the master genuinely
/// changing tempo, which is the case a plain outlier filter gets wrong.
class MidiClockReceiver {
public:
    static constexpr Tick ticksPerPulse = MidiClockGenerator::ticksPerPulse;
    static constexpr Tick ticksPerSongPositionBeat = MidiClockGenerator::ticksPerSongPositionBeat;

    /// Accepted intervals before the estimate is called settled. 24 is one
    /// quarter note: long enough to average the bus slop out, short enough
    /// that a user pressing play on the master does not wait for it.
    static constexpr int pulsesToLock = 24;

    /// How far an interval may be from the running estimate and still be
    /// treated as slop rather than as a tempo change.
    static constexpr double toleranceRatio = 0.30;

    /// Consecutive rejections that mean the master really did change tempo.
    static constexpr int rejectionsBeforeReseed = 8;

    /// Intervals averaged, unfiltered, before the tolerance test starts.
    ///
    /// Seeding from a single interval hands the filter whatever slop that one
    /// packet carried, and the tolerance test then defends that number against
    /// the truth: a clock alternating either side of its nominal rate would
    /// settle on one extreme and reject the other forever. A short mean has no
    /// side to take.
    static constexpr int seedIntervals = 8;

    /// Smoothing factor for the running estimate. One sixteenth settles
    /// within a beat or so and still rides out a single late packet.
    static constexpr double smoothing = 1.0 / 16.0;

    /// No pulse for this long means the master has gone away. The transport
    /// is left alone — only an explicit stop stops it — but the estimate stops
    /// being called locked, because it is no longer being measured.
    static constexpr double dropoutSeconds = 1.0;

    void setRole(MidiClockRole role) noexcept { role_.store(role, std::memory_order_relaxed); }
    [[nodiscard]] MidiClockRole role() const noexcept { return role_.load(std::memory_order_relaxed); }

    void reset() noexcept;

    /// Audio thread. Reads this block's incoming messages and drives
    /// `transport`. Realtime-safe: transport control is atomic stores, and the
    /// filter is a handful of arithmetic operations per pulse.
    void process(const MidiBuffer& incoming,
                 Transport&        transport,
                 FrameCount        blockSize,
                 SampleRate        sampleRate) noexcept;

    /// The measured tempo, or 0 before the estimate settles.
    [[nodiscard]] double estimatedTempo() const noexcept
    {
        return estimatedTempo_.load(std::memory_order_relaxed);
    }

    /// True while pulses are arriving and the estimate has settled.
    [[nodiscard]] bool isLocked() const noexcept { return locked_.load(std::memory_order_relaxed); }

    /// The musical position the incoming clock reports, in ticks. Advances 40
    /// ticks per pulse from the last start or song position pointer.
    [[nodiscard]] Tick externalTick() const noexcept
    {
        return externalTick_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t pulseCount()    const noexcept { return pulses_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t rejectedCount() const noexcept { return rejected_.load(std::memory_order_relaxed); }

private:
    void acceptInterval(double intervalFrames, SampleRate sampleRate) noexcept;

    std::atomic<MidiClockRole> role_{MidiClockRole::off};

    std::atomic<double>        estimatedTempo_{0.0};
    std::atomic<bool>          locked_{false};
    std::atomic<Tick>          externalTick_{0};
    std::atomic<std::uint64_t> pulses_{0};
    std::atomic<std::uint64_t> rejected_{0};

    /// Absolute frames since `reset`, so an interval can be measured across a
    /// block boundary.
    FrameCount elapsedFrames_ = 0;

    FrameCount lastPulseFrame_ = 0;
    bool       hasPulse_       = false;

    double intervalFrames_  = 0.0;   ///< the running estimate, 0 until seeded
    int    acceptedRun_     = 0;
    int    rejectedRun_     = 0;

    double seedSum_   = 0.0;
    int    seedCount_ = 0;
};

} // namespace incdaw::engine
