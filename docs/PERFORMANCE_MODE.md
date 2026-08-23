# INCDAW — Performance Mode

**Status:** increment 2 implemented (`engine::PerformanceScheduler`, headless and
tested). Increments 3–6 are still design.

TRACK B item B12 asks for a written plan before any code, and names the
scheduling design as the deliverable of the first increment. This is that
deliverable.

---

## 1. What the feature is

FL Studio 2026's Performance Mode turns the playlist into an instrument. The
region *before the start marker* becomes a performance zone; the clips in it
are no longer played by the transport in sequence but **triggered live**, one
clip at a time per track, from a pad controller or the typing keyboard.

Each track carries its own behaviour:

| Setting | Values | Meaning |
|---|---|---|
| Press | retrigger · hold · hold-and-stop · hold-and-motion · latch | what a press and a release do |
| Motion | stay · one-shot · march-and-wrap · march-and-stay · march-and-stop · random · exclusive random | which clip the *next* trigger selects |
| Trigger sync | off · 1/4 · 1/2 · 1 bar · 2 · 4 · … | the beat boundary a trigger takes effect on |
| Position sync | off · from start · from the bar it lands in | where inside the clip playback begins |

Two properties make this hard rather than fiddly:

1. **Triggers arrive at arbitrary times and take effect at musical
   boundaries.** A pad hit 40 ms before the bar line must start the clip *on*
   the bar line, sample-accurately, not at the top of the next audio block.
2. **The set of sounding clips changes while the graph is running.** The
   transport today plays a compiled arrangement from a position; this asks it
   to start and stop clips from a live input without recompiling the graph per
   trigger. A recompile allocates, and allocation is not available on the audio
   thread (CLAUDE.md §3).

---

## 2. What already exists that this builds on

- `engine::Transport::processBlock` already **splits a block into segments** at
  loop wraps, with a documented bound (`maxSegmentsPerBlock`) and no
  allocation. Sample-accurate boundaries inside a block are therefore an
  established shape in this codebase, not a new one.
- `engine::AudioClipNode` already holds every audio clip of one track and plays
  from `playPosition` by pure indexing, with no state advancing between blocks.
- `project::compileArrangement` already resolves *which placements* are
  compiled — mute, solo, and now folder propagation — off the audio thread.
- The graph is swapped by an **atomic pointer** (docs/ARCHITECTURE.md §4, §7),
  and the old graph is released off the realtime thread by the reaper.
- Arrangements (format 1.11) already give a project more than one timeline, and
  the performance zone is a region of one of them.

---

## 3. The core decision: a scene table, not a graph rebuild

**A trigger must not rebuild the graph.** The design is instead:

> The compiler builds, once, a graph containing **every clip in the performance
> zone**, all of them silent. A small realtime-safe **scene table** decides,
> per block, which of them are sounding and from which frame. Triggers write
> into that table; the audio thread reads it.

Concretely:

- A new `engine::PerformanceScheduler`, owned by the compiled graph, holds a
  fixed-capacity array of **track slots** (one per playlist track in the zone).
  Each slot holds: the clip index currently armed, the clip currently playing,
  the frame the current clip started at, and the track's press/motion settings.
  Fixed capacity, sized at compile time from the project — so no allocation and
  no unbounded work in `process`.
- `AudioClipNode` and the pattern/instrument path gain an optional
  **`PerformanceSource`** pointer: when set, the node asks the scheduler which
  clip of its track is sounding and at what offset, instead of reading the
  transport position directly. When null — every existing project — the node
  behaves exactly as it does today. This is the same shape as the disk streamer:
  an optional pointer whose absence is the current behaviour.
- Triggers arrive on the **control thread** (MIDI input, key events) and are
  posted into a single-producer/single-consumer **ring buffer** of
  `{track, clipIndex, pressed, hostTimeNanos}`. The audio thread drains the ring
  at the top of each block. Nothing is allocated, nothing is locked, and a full
  ring drops the oldest trigger rather than blocking — a dropped pad hit is
  recoverable, a blocked audio thread is not.

### Why not the alternatives

- **Recompile per trigger.** Correct and simple, and unusable: a compile
  allocates, loads assets and walks the whole project. Even off-thread with a
  swap it is tens of milliseconds late, and Performance Mode is played to a
  beat.
- **One graph per possible scene.** The combinatorics are hopeless: eight
  tracks with eight clips each is 16.7 million graphs.
- **Mute automation on every clip.** This is the scene table wearing a disguise,
  but routed through the automation system, which is per-parameter and
  tick-based rather than per-clip and trigger-based. It would also make every
  performance an undoable edit, which is the wrong model — a performance is
  played, not authored.

---

## 4. Quantised triggers, sample-accurately

A trigger carries the host time it happened at. The audio thread converts it to
a timeline frame through the same anchor a recorded take uses
(`RecordingSession`'s host-time → timeline-frame mapping, already tested), then
rounds it **forward** to the track's trigger-sync boundary:

```
effectiveFrame = tempoMap.frameForTick(ceilToGrid(tempoMap.tickForFrame(frame), syncTicks))
```

The scheduler then holds the trigger as **pending** until a block contains that
frame, and the block is split there — the same segment mechanism the transport
already uses for loop wraps. A clip therefore starts on the exact frame of the
bar line, not at the top of the block containing it.

Bound: the ring itself. Triggers **wait in the ring** until their quantised
frame is due, and each block scans the unconsumed ones — in the order they were
posted — applying those that have come due, then walks the read cursor over the
consumed prefix.

> **Corrected during increment 2.** The first design drained the ring into a
> single pending slot per track, on the reasoning that a second press before
> the first has landed replaces it. That is right for two presses and wrong for
> a press and its release: queue both before either is due and the release
> overwrites the press, so the clip never sounds at all. The ring already had
> the room and the ordering; what it needed was the patience. Two presses still
> resolve to the later one, because both apply on the same quantised frame and
> the second one lands last.

Scanning rather than popping the head also matters: trigger grids are per
track, so the ring is not in due-order, and popping only the head would let one
track's un-due trigger block another track's due one.

---

## 5. What each behaviour becomes

- **Press: retrigger** — start at `effectiveFrame`; a second press restarts.
- **Press: hold** — start on press, stop on release (quantised too, or the
  release would be ragged).
- **Press: hold-and-stop / hold-and-motion / latch** — all are the same state
  machine with a different release action and a different motion trigger; they
  are a `switch` in the slot's update, not four code paths.
- **Motion: stay / one-shot / march-* / random / exclusive random** — motion
  chooses the *next* clip index for the slot when the current one ends. Random
  needs a deterministic source: the graph already carries `randomSeed`
  (`GraphCompileOptions`), so a performance is reproducible in an offline
  render, which is what makes recording one possible later.
- **Position sync** — decides the source offset the clip starts at: zero, or
  the offset matching the bar the trigger landed in.

---

## 6. The model and the format

Performance settings are project state, so they are saved:

- `Track::performancePress`, `Track::performanceMotion`, `Track::triggerSync`,
  `Track::positionSync` — four small enums on the track.
- `Clip::performanceKey` — the pad or key that triggers this clip, so a layout
  survives reopening.
- The performance zone itself needs no field: it is the region before the
  arrangement's start marker, and markers are already per-arrangement.

That is **one format bump, additive**, with every default meaning "behaves as
it does today". It should be spent in the first implementing increment, not
this one.

---

## 7. Increments

1. **This document**, plus the decision record (D-041).
2. ~~**The scheduler, headless.**~~ **Done.** `PerformanceScheduler` with its
   slot table, trigger ring and quantisation, tested without any audio: 21
   cases covering every press and motion behaviour, forward-only quantisation,
   position sync, deterministic randomness, a full ring, a zero-length clip,
   and a realtime-safety case asserting that a block of triggers allocates
   nothing and takes no lock. One design correction came out of it, recorded
   in §4.
3. **The engine path.** `PerformanceSource` on `AudioClipNode` and the
   instrument path; a realtime-safety test that a block with triggers pending
   allocates nothing and takes no lock.
4. **The model and format.** The four track enums, the clip key, the bump and
   the migration.
5. **The surface.** The zone drawn in the playlist, per-track behaviour
   controls, and the typing-keyboard map.
6. **Controller triggering**, through the existing MIDI mapping system, which
   already binds hardware controls to parameters by registry key.

Increments 2 and 3 are the risky ones and are deliberately first. Nothing in
1–3 changes an existing project or an existing render.

---

## 8. What this design does not yet answer

Stated rather than hidden:

- **Recording a performance back into the arrangement.** The trigger log makes
  it possible in principle; the placement rules are not designed.
- **Pattern clips versus audio clips in the zone.** Audio clips are the
  straightforward case. A triggered *pattern* clip has to start an instrument
  mid-phrase, which interacts with note-off handling in a way that needs its
  own test before it is promised.
- **Latency compensation.** A triggered clip on a track whose strip carries a
  plugin with latency will sound late by that latency. PDC is applied to the
  graph, not to the trigger, and the correct fix is to advance the trigger's
  effective frame by the track's reported latency. It is one line, and it is
  one line that needs a test.
