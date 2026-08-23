#pragma once

#include "engine/core/Time.h"
#include "engine/transport/TempoMap.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace incdaw::engine {

/// What a press and a release do on one performance track.
enum class PerformancePress : std::uint8_t {
    retrigger,       ///< a press starts the clip; a second press restarts it
    hold,            ///< sounds while the pad is down
    holdAndStop,     ///< sounds while down, and the release stops the track
    holdAndMotion,   ///< sounds while down, and the release advances the motion
    latch,           ///< a press starts it, the next press stops it
};

/// Which clip the next trigger on a track selects.
enum class PerformanceMotion : std::uint8_t {
    stay,             ///< the pad's own clip, every time
    oneShot,          ///< plays once and stops
    marchAndWrap,     ///< the next clip each time, wrapping to the first
    marchAndStay,     ///< the next clip each time, holding on the last
    marchAndStop,     ///< the next clip each time, stopping after the last
    random,           ///< any clip on the track
    exclusiveRandom,  ///< any clip except the one that just played
};

/// Decides, per block, which clip of each performance track is sounding.
///
/// The whole of docs/PERFORMANCE_MODE.md §3 in one class. The compiler builds
/// ONE graph holding every clip in the performance zone, all of them silent,
/// and this table says which are heard — so a trigger never rebuilds a graph,
/// never allocates, and never takes a lock.
///
/// The division of labour is deliberate: this class knows about slots, clips,
/// frames and behaviours, and nothing about audio, nodes or the project. It is
/// therefore testable by feeding it triggers and asking what is sounding, which
/// is where the design is proved or discarded before any node depends on it.
///
/// Threading: `prepare` is non-realtime and must not run while the audio thread
/// is using the scheduler. `postTrigger` is called by ONE control thread.
/// `advanceTo`, `voiceAt` and `nextBoundaryIn` are called by the audio thread
/// only. That is a single-producer, single-consumer arrangement, which is what
/// lets the trigger queue be a lock-free ring.
class PerformanceScheduler {
public:
    /// How many triggers can be in flight. A full ring drops the OLDEST rather
    /// than blocking: a dropped pad hit is recoverable, a blocked audio thread
    /// is not.
    static constexpr std::size_t triggerCapacity = 256;

    /// One clip's placement inside the performance zone.
    struct ClipPlacement {
        FramePosition start  = 0;   ///< where the clip sits on the timeline
        FrameCount    length = 0;
    };

    /// One track's clips and behaviour.
    struct TrackSetup {
        PerformancePress   press        = PerformancePress::retrigger;
        PerformanceMotion  motion       = PerformanceMotion::stay;

        /// The grid a trigger is rounded FORWARD to, in ticks. Zero triggers
        /// on the frame the pad was hit, which is what "off" means.
        Tick               syncTicks    = 0;

        /// Whether a clip starts from its own beginning (false) or from the
        /// point of it that matches where the trigger landed (true).
        bool               positionSync = false;

        std::vector<ClipPlacement> clips;
    };

    /// What a track is sounding at a given frame.
    struct Voice {
        bool        sounding    = false;
        std::size_t clip        = 0;
        /// Frames into the clip's own material.
        FrameCount  sourceFrame = 0;
    };

    // ── Non-realtime ────────────────────────────────────────────────────────

    /// Sizes the table and clears it. Allocates; call it while the audio thread
    /// is not using this scheduler, exactly as a graph rebuild is.
    void prepare(std::vector<TrackSetup> tracks);

    /// Silences every track and drops every pending trigger, keeping the setup.
    void reset() noexcept;

    [[nodiscard]] std::size_t trackCount() const noexcept { return tracks_.size(); }

    /// Deterministic randomness, so a performance can be reproduced in an
    /// offline render. Seeded from the graph's own seed.
    void setRandomSeed(std::uint64_t seed) noexcept { random_ = seed == 0 ? 1u : seed; }

    // ── Control thread ──────────────────────────────────────────────────────

    /// Queues a press or a release. `frame` is the timeline frame it happened
    /// at, which the caller has already resolved from host time.
    ///
    /// Returns false only when the ring was full and the oldest trigger had to
    /// be dropped — the trigger itself is still queued.
    bool postTrigger(std::size_t track, std::size_t clip, bool pressed,
                     FramePosition frame) noexcept;

    // ── Audio thread ────────────────────────────────────────────────────────

    /// Drains the trigger ring and applies everything that has come due at or
    /// before `frame`. Bounded: at most `triggerCapacity` triggers and one
    /// state change per track.
    void advanceTo(FramePosition frame, const TempoMap& tempoMap) noexcept;

    [[nodiscard]] Voice voiceAt(std::size_t track, FramePosition frame) const noexcept;

    /// The next frame in [from, from + length) at which something changes on
    /// any track, or `from + length` when nothing does — the frame a block
    /// should be split at, the same way the transport splits at a loop wrap.
    [[nodiscard]] FramePosition nextBoundaryIn(FramePosition from,
                                               FrameCount length) const noexcept;

private:
    struct Slot {
        TrackSetup setup;

        bool          playing     = false;
        std::size_t   playingClip = 0;
        FramePosition startedAt   = 0;

        /// Where in the clip's own material playback began, which position
        /// sync is the only thing that makes non-zero.
        FrameCount    sourceOffset = 0;

        bool          held        = false;
        std::size_t   heldClip    = 0;

        bool          exhausted   = false;   ///< march-and-stop has run off the end
    };

    void apply(Slot& slot, std::size_t clip, bool pressed, FramePosition frame) noexcept;
    void startClip(Slot& slot, std::size_t clip, FramePosition frame) noexcept;
    void endClip(Slot& slot) noexcept;
    void advanceMotion(Slot& slot, FramePosition frame) noexcept;

    [[nodiscard]] FramePosition quantise(const Slot& slot, FramePosition frame,
                                         const TempoMap& tempoMap) const noexcept;

    [[nodiscard]] std::size_t rollClip(const Slot& slot) noexcept;

    /// A queued trigger.
    ///
    /// Triggers wait IN THE RING until their quantised frame is due, rather
    /// than being drained into a per-track pending slot. One pending slot per
    /// track cannot hold a press and its release when both are queued before
    /// either is due — the release would overwrite the press and the note
    /// would never sound. The ring already has the room and already has the
    /// ordering; the only thing it needed was the patience.
    struct Trigger {
        std::size_t   track     = 0;
        std::size_t   clip      = 0;
        bool          pressed   = false;
        FramePosition frame     = 0;

        /// The quantised frame, worked out once when the trigger is first
        /// looked at, and `consumed` once it has been applied.
        FramePosition effective = 0;
        bool          resolved  = false;
        bool          consumed  = true;
    };

    std::vector<Slot> tracks_;

    Trigger                   ring_[triggerCapacity]{};
    std::atomic<std::size_t>  write_{0};
    std::atomic<std::size_t>  read_{0};

    std::uint64_t random_ = 0x9E3779B97F4A7C15ull;
};

} // namespace incdaw::engine
