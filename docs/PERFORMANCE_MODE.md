# INCDAW — Performance Mode

**Status:** increments 2–5 implemented. A performance can be set up in the
playlist and played from the typing keyboard. Increment 6 — triggering from a
MIDI controller — is still design, and is blocked on a file boundary this track
does not own (see below).

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

- `Track::performancePress`, `Track::performanceMotion`,
  `Track::triggerSyncTicks`, `Track::positionSync`. These reuse the ENGINE's
  own enums rather than mirroring them: `project/` can see `engine/` — it
  already does for the tempo map — so there is nothing to translate and
  nothing to keep in step. (`AutomationCurve` is mirrored because the
  dependency runs the other way there.)
- `Clip::performanceKey` — the pad or key that triggers this clip, so a layout
  survives reopening. -1 means nothing triggers it.
- `TimelineMarker::isStart` — the performance zone needs no field of its own,
  because it is the region before the arrangement's start marker and markers
  are already per-arrangement. At most one per arrangement, which the command
  enforces the way "one current arrangement" is enforced.

That is **one format bump, additive** (1.12), with every default meaning
"behaves as it does today".

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
3. ~~**The engine path.**~~ **Done for audio clips.** `AudioClipNode` takes an
   optional scheduler — absent, it behaves exactly as every existing project
   needs it to — and splits its own block at the table's boundaries, so a clip
   triggered on a beat starts on that frame rather than at the top of the
   block containing it. Asserted sample by sample, along with a mid-block
   motion handover, a mid-block release, and the realtime contract.

   The instrument path is **not** done: a triggered *pattern* clip has to start
   an instrument mid-phrase, which is the note-off question §8 already flags.

   One design point was settled here. `advanceTo` is **monotonic**: a call with
   a frame the table has already passed does nothing. That is what lets several
   nodes read one table without any of them being nominated its driver —
   whichever reaches a frame first does the work and the rest read the result,
   so node order in the graph does not matter. The alternative was teaching the
   graph itself about performances, which would have put a feature-specific
   concept in the one place that has none. `rewindTo` exists for the other side
   of that rule: a seek or a loop wrap has to be able to move the table
   backwards.
4. ~~**The model and format.**~~ **Done.** The four track settings, the clip's
   pad, and `TimelineMarker::isStart` — the start marker being what makes the
   region before it a zone, so the zone needed no field of its own. Format
   1.12, additive, with the migration and a frozen fixture. The compiler
   builds the scene table from all of it and points the nodes at it, behind
   `GraphCompileOptions::performanceMode`, which is off by default: a start
   marker alone changes nothing until the mode is switched on.
5. ~~**The surface.**~~ **Done.** The zone washed in the playlist with the
   start marker as its edge; the mode toggled from the arrangement menu, which
   refuses until a start marker exists; per-track press, motion, trigger sync
   and position sync; per-clip pad, badged on the clip so a layout can be read
   off the arrangement; and the number row as the pad bank, pressing on keyDown
   and releasing on keyUp, only while the mode is on.

   One pad drives one clip **per track**, so a single key starts a whole scene
   — which is what makes eight keys enough to play with.

   Which clip a pad resolves to goes through the mapping the compiler recorded
   (`CompiledProjectGraph::performanceClips`) rather than being worked out
   again. The clip order behind a slot is the compiler's — lane, then start,
   then id — and a second implementation of that rule would eventually disagree
   and trigger the wrong clip with nothing to say so.
6. **Controller triggering.** Not done, and **blocked on ownership rather than
   on design**: the MIDI mapping system binds hardware controls to *parameters*
   by registry key, and a pad is not a parameter, so it needs a second kind of
   mapping and a hook in the MIDI input path — which lives in `src/platform/`,
   a directory TRACK B does not own. The scheduler side needs nothing new: the
   trigger ring is already single-producer and already takes a timeline frame,
   so a MIDI thread can post into it exactly as the window does.

Increments 2 and 3 were the risky ones and were deliberately first. Nothing in
1–5 changes an existing project or an existing render: the mode is off by
default, and with it off a start marker is a marker like any other.

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
