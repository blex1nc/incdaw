#include "engine/performance/PerformanceScheduler.h"

#include <algorithm>
#include <utility>

namespace incdaw::engine {

// ── Non-realtime ─────────────────────────────────────────────────────────────

void PerformanceScheduler::prepare(std::vector<TrackSetup> tracks)
{
    tracks_.clear();
    tracks_.reserve(tracks.size());

    for (TrackSetup& setup : tracks) {
        Slot slot;
        slot.setup = std::move(setup);
        tracks_.push_back(std::move(slot));
    }

    write_.store(0, std::memory_order_relaxed);
    read_.store(0, std::memory_order_relaxed);
}

void PerformanceScheduler::reset() noexcept
{
    for (Slot& slot : tracks_) {
        slot.playing      = false;
        slot.held         = false;
        slot.sourceOffset = 0;
        slot.exhausted    = false;
    }

    read_.store(write_.load(std::memory_order_acquire), std::memory_order_release);
}

// ── Control thread ───────────────────────────────────────────────────────────

bool PerformanceScheduler::postTrigger(std::size_t track, std::size_t clip, bool pressed,
                                       FramePosition frame) noexcept
{
    if (track >= tracks_.size())
        return true;   // nothing to trigger; not a dropped trigger either

    const std::size_t write = write_.load(std::memory_order_relaxed);
    const std::size_t read  = read_.load(std::memory_order_acquire);

    bool dropped = false;

    // Full: drop the oldest by walking the read cursor on. Only this thread
    // ever advances `write_`, and the audio thread's own advance of `read_` can
    // only make more room, so racing here can lose a trigger but cannot corrupt
    // the ring.
    if (write - read >= triggerCapacity) {
        read_.store(read + 1, std::memory_order_release);
        dropped = true;
    }

    ring_[write % triggerCapacity] = Trigger{track, clip, pressed, frame, 0, false, false};
    write_.store(write + 1, std::memory_order_release);

    return !dropped;
}

// ── Audio thread ─────────────────────────────────────────────────────────────

FramePosition PerformanceScheduler::quantise(const Slot& slot, FramePosition frame,
                                             const TempoMap& tempoMap) const noexcept
{
    if (slot.setup.syncTicks <= 0)
        return frame;

    const Tick tick = tempoMap.tickForFrame(frame);
    const Tick grid = slot.setup.syncTicks;

    // Forward, always: a pad hit just before the bar line means the bar line,
    // and a pad hit just after it means the NEXT one — playing early is the one
    // thing a quantised trigger must never do.
    const Tick remainder = tick % grid;
    const Tick rounded   = remainder == 0 ? tick : tick + (grid - remainder);

    return tempoMap.frameForTick(rounded);
}

std::size_t PerformanceScheduler::rollClip(const Slot& slot) noexcept
{
    const std::size_t count = slot.setup.clips.size();
    if (count == 0)
        return 0;

    // xorshift64*: deterministic from the seed, no allocation, and the same
    // sequence in an offline render as in the live performance.
    random_ ^= random_ >> 12;
    random_ ^= random_ << 25;
    random_ ^= random_ >> 27;

    const auto roll = static_cast<std::size_t>((random_ * 0x2545F4914F6CDD1Dull) >> 33);

    if (slot.setup.motion == PerformanceMotion::exclusiveRandom && count > 1) {
        // One of the others, chosen without a retry loop: an unbounded retry
        // is exactly the shape the audio thread cannot have.
        const std::size_t other = roll % (count - 1);
        return other >= slot.playingClip ? other + 1 : other;
    }

    return roll % count;
}

void PerformanceScheduler::startClip(Slot& slot, std::size_t clip, FramePosition frame) noexcept
{
    if (clip >= slot.setup.clips.size())
        return;

    slot.playing     = true;
    slot.playingClip = clip;
    slot.startedAt   = frame;

    // Position sync starts the clip at the point of it matching where the
    // trigger landed relative to the clip's own place on the timeline, so a
    // clip joined mid-bar stays in phase with everything else.
    if (slot.setup.positionSync) {
        const FramePosition placed = slot.setup.clips[clip].start;
        const FrameCount    length = slot.setup.clips[clip].length;

        if (length > 0) {
            const auto into = frame > placed ? static_cast<FrameCount>(frame - placed) : 0;
            slot.sourceOffset = length > 0 ? into % length : 0;
        } else {
            slot.sourceOffset = 0;
        }
    } else {
        slot.sourceOffset = 0;
    }
}

void PerformanceScheduler::endClip(Slot& slot) noexcept
{
    slot.playing      = false;
    slot.sourceOffset = 0;
}

void PerformanceScheduler::advanceMotion(Slot& slot, FramePosition frame) noexcept
{
    const std::size_t count = slot.setup.clips.size();
    if (count == 0) {
        endClip(slot);
        return;
    }

    switch (slot.setup.motion) {
        case PerformanceMotion::stay:
            startClip(slot, slot.playingClip, frame);
            return;

        case PerformanceMotion::oneShot:
            endClip(slot);
            return;

        case PerformanceMotion::marchAndWrap:
            startClip(slot, (slot.playingClip + 1) % count, frame);
            return;

        case PerformanceMotion::marchAndStay:
            startClip(slot, std::min(slot.playingClip + 1, count - 1), frame);
            return;

        case PerformanceMotion::marchAndStop:
            if (slot.playingClip + 1 >= count) {
                slot.exhausted = true;
                endClip(slot);
                return;
            }

            startClip(slot, slot.playingClip + 1, frame);
            return;

        case PerformanceMotion::random:
        case PerformanceMotion::exclusiveRandom:
            startClip(slot, rollClip(slot), frame);
            return;
    }
}

void PerformanceScheduler::apply(Slot& slot, std::size_t clip, bool pressed,
                                 FramePosition frame) noexcept
{
    if (clip >= slot.setup.clips.size())
        return;

    if (pressed) {
        slot.held      = true;
        slot.heldClip  = clip;
        slot.exhausted = false;

        // Latch is the one press that can mean "stop": pressing the pad that
        // is already sounding takes it away again.
        if (slot.setup.press == PerformancePress::latch && slot.playing
            && slot.playingClip == clip) {
            endClip(slot);
            return;
        }

        startClip(slot, clip, frame);
        return;
    }

    slot.held = false;

    switch (slot.setup.press) {
        case PerformancePress::retrigger:
        case PerformancePress::latch:
            return;   // the release means nothing

        case PerformancePress::hold:
        case PerformancePress::holdAndStop:
            if (slot.playing && slot.playingClip == clip)
                endClip(slot);
            return;

        case PerformancePress::holdAndMotion:
            if (slot.playing && slot.playingClip == clip)
                advanceMotion(slot, frame);
            return;
    }
}

void PerformanceScheduler::advanceTo(FramePosition frame, const TempoMap& tempoMap) noexcept
{
    const std::size_t write = write_.load(std::memory_order_acquire);
    std::size_t       read  = read_.load(std::memory_order_relaxed);

    // Every unconsumed trigger is looked at, in the order it was posted, and
    // applied if it has come due. Scanning rather than popping the head is what
    // stops one track's un-due trigger from blocking another track's due one:
    // the grids are per track, so the ring is not in due-order.
    for (std::size_t index = read; index != write; ++index) {
        Trigger& trigger = ring_[index % triggerCapacity];

        if (trigger.consumed || trigger.track >= tracks_.size()) {
            trigger.consumed = true;
            continue;
        }

        Slot& slot = tracks_[trigger.track];

        if (!trigger.resolved) {
            trigger.effective = quantise(slot, trigger.frame, tempoMap);
            trigger.resolved  = true;
        }

        if (frame < trigger.effective)
            continue;

        apply(slot, trigger.clip, trigger.pressed, trigger.effective);
        trigger.consumed = true;
    }

    // The read cursor only moves over a consumed prefix, so a trigger waiting
    // for a later bar keeps its place and its ordering.
    while (read != write && ring_[read % triggerCapacity].consumed)
        ++read;

    read_.store(read, std::memory_order_release);

    for (Slot& slot : tracks_) {
        // A clip that has run out either stops or hands over, depending on the
        // track's motion. One handover per call, which is what keeps this
        // bounded: a zero-length clip cannot spin.
        if (!slot.playing)
            continue;

        const ClipPlacement& clip = slot.setup.clips[slot.playingClip];
        if (clip.length == 0 || clip.length <= slot.sourceOffset)
            continue;

        const FramePosition ends =
            slot.startedAt + static_cast<FramePosition>(clip.length - slot.sourceOffset);

        if (frame >= ends)
            advanceMotion(slot, ends);
    }
}

PerformanceScheduler::Voice PerformanceScheduler::voiceAt(std::size_t track,
                                                          FramePosition frame) const noexcept
{
    Voice voice;

    if (track >= tracks_.size())
        return voice;

    const Slot& slot = tracks_[track];
    if (!slot.playing || frame < slot.startedAt)
        return voice;

    const ClipPlacement& clip = slot.setup.clips[slot.playingClip];

    const auto into = static_cast<FrameCount>(frame - slot.startedAt) + slot.sourceOffset;
    if (clip.length > 0 && into >= clip.length)
        return voice;

    voice.sounding    = true;
    voice.clip        = slot.playingClip;
    voice.sourceFrame = into;
    return voice;
}

FramePosition PerformanceScheduler::nextBoundaryIn(FramePosition from,
                                                   FrameCount length) const noexcept
{
    const FramePosition end = from + static_cast<FramePosition>(length);
    FramePosition       next = end;

    const std::size_t write = write_.load(std::memory_order_acquire);

    for (std::size_t index = read_.load(std::memory_order_relaxed); index != write; ++index) {
        const Trigger& trigger = ring_[index % triggerCapacity];

        if (!trigger.consumed && trigger.resolved
            && trigger.effective > from && trigger.effective < next)
            next = trigger.effective;
    }

    for (const Slot& slot : tracks_) {
        if (!slot.playing)
            continue;

        const ClipPlacement& clip = slot.setup.clips[slot.playingClip];
        if (clip.length == 0 || clip.length <= slot.sourceOffset)
            continue;

        const FramePosition ends =
            slot.startedAt + static_cast<FramePosition>(clip.length - slot.sourceOffset);

        if (ends > from && ends < next)
            next = ends;
    }

    return next;
}

} // namespace incdaw::engine
