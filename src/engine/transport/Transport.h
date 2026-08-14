#pragma once

#include "engine/transport/TempoMap.h"

#include <atomic>
#include <cstddef>

namespace incdaw::engine {

enum class TransportState {
    stopped,
    playing,
    recording,
};

/// One contiguous run of frames within a processing block over which the
/// timeline advances linearly.
///
/// A block is split whenever something discontinuous happens inside it — today
/// a loop wrap, later also tempo changes and punch points. Nodes render each
/// segment separately, which is what makes events land on their exact frame
/// instead of being rounded to the block boundary (docs/AUDIO_ENGINE.md §6).
struct BlockSegment {
    FrameCount    offset       = 0;   ///< frames into the block
    FrameCount    length       = 0;
    FramePosition startFrame   = 0;   ///< timeline position of this segment's first frame
    bool          startsAfterLoopWrap = false;
};

/// The single authority for time.
///
/// MIDI scheduling, clip playback, automation and rendering all read position
/// from here. docs/AUDIO_ENGINE.md §5: there is no second clock anywhere in the
/// system, because two clocks eventually disagree and the disagreement is
/// exactly what a listener hears.
///
/// Control changes (play, stop, seek, loop) are published by the UI thread as
/// atomics and applied by the audio thread at the start of the next block. That
/// costs up to one block of latency on a transport command — inaudible, and far
/// cheaper than the alternative of a lock shared with the audio thread.
class Transport {
public:
    /// The most segments one block can be split into.
    ///
    /// A block can wrap the loop repeatedly only if the loop is shorter than the
    /// block. Eight is generous for that case and keeps the plan on the stack.
    static constexpr std::size_t maxSegmentsPerBlock = 8;

    explicit Transport(TempoMap tempoMap = TempoMap{});

    // ── Control (non-realtime side) ──────────────────────────────────────────

    void play() noexcept;
    void stop() noexcept;
    void pause() noexcept;
    void startRecording() noexcept;

    /// Requests a jump. Applied at the start of the next block.
    void seek(FramePosition frame) noexcept;
    void seekToTick(Tick tick) noexcept { seek(tempoMap_.frameForTick(tick)); }

    void setLoopEnabled(bool enabled) noexcept { loopEnabled_.store(enabled, std::memory_order_relaxed); }
    void setLoopRange(FramePosition start, FramePosition end) noexcept;

    [[nodiscard]] bool          isLoopEnabled() const noexcept { return loopEnabled_.load(std::memory_order_relaxed); }
    [[nodiscard]] FramePosition loopStart()     const noexcept { return loopStart_.load(std::memory_order_relaxed); }
    [[nodiscard]] FramePosition loopEnd()       const noexcept { return loopEnd_.load(std::memory_order_relaxed); }

    /// The tempo map. Replacing it is a non-realtime operation and must not be
    /// done while the audio thread is running; the project layer routes such
    /// edits through a graph rebuild.
    [[nodiscard]] const TempoMap& tempoMap() const noexcept { return tempoMap_; }
    void setTempoMap(TempoMap map);

    /// Mutable access, for setting the sample rate once the device is open.
    /// Not safe while the audio thread is running — the project layer routes
    /// tempo edits through a graph rebuild.
    [[nodiscard]] TempoMap& tempoMapForEdit() noexcept { return tempoMap_; }

    // ── Queries (safe from either side) ──────────────────────────────────────

    [[nodiscard]] TransportState state()    const noexcept { return state_.load(std::memory_order_acquire); }
    [[nodiscard]] bool           isPlaying() const noexcept { return state() != TransportState::stopped; }

    [[nodiscard]] FramePosition position() const noexcept { return position_.load(std::memory_order_acquire); }
    [[nodiscard]] Tick          positionInTicks() const noexcept { return tempoMap_.tickForFrame(position()); }
    [[nodiscard]] MusicalPosition musicalPosition() const noexcept
    {
        return tempoMap_.musicalPositionForTick(positionInTicks());
    }

    // ── Realtime side ────────────────────────────────────────────────────────

    /// Plans one block and advances the transport past it.
    ///
    /// Writes at most `maxSegmentsPerBlock` segments into `segments` and returns
    /// how many were written. When stopped, returns a single segment at the
    /// current position with the transport not advancing, so that nodes still
    /// render (a synth must keep sounding when the transport stops).
    ///
    /// Realtime-safe: no allocation, no locks, bounded work.
    [[nodiscard]] std::size_t processBlock(FrameCount blockSize,
                                           BlockSegment* segments,
                                           std::size_t   maxSegments) noexcept;

private:
    void applyPendingSeek() noexcept;

    TempoMap tempoMap_;

    std::atomic<TransportState> state_{TransportState::stopped};
    std::atomic<FramePosition>  position_{0};

    std::atomic<bool>          seekRequested_{false};
    std::atomic<FramePosition> seekTarget_{0};

    std::atomic<bool>          loopEnabled_{false};
    std::atomic<FramePosition> loopStart_{0};
    std::atomic<FramePosition> loopEnd_{0};
};

} // namespace incdaw::engine
