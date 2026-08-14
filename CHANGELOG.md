# INCDAW — Changelog

All notable changes to this project are recorded here.
Format loosely follows Keep a Changelog. The project is pre-release; there is no
public version yet.

---

## [Unreleased]

### Phase 8 — Pattern system — 2026-08-14

**Added**

- `project/GraphCompiler` — the single project → render-graph compiler. One
  instrument node and one channel strip per channel, into a master gain, built
  on a non-realtime thread and installed with one atomic swap (D-013).
- `project/InstrumentFactory` — turns a channel's persisted plugin identifier
  into an instrument. A format INCDAW cannot host yet renders silence and keeps
  its place in the graph rather than being dropped from the project.
- `engine/dsp/ChannelStripNode` — a channel's own volume, pan and mute, with a
  constant-power pan law and smoothed gains. Not the mixer; Phase 10 sits
  downstream of it.
- Multi-channel patterns: a note carries the id of the channel that plays it
  (D-012). One pattern holds a whole kit.
- Polymetric patterns: a channel may loop at a shorter length than the pattern
  it lives in, so a three-step figure drifts across a sixteen-step bar.
- Swing, pattern-wide or per channel, applied when the pattern is compiled and
  never written into the notes — so turning it off restores the exact timing.
- Song mode: `compileArrangement` flattens the pattern clips on the timeline
  into what each channel plays. Every placement of a pattern is compiled with
  the same seed, so probability cannot make two placements differ.
- `app/StepSequencerModel` — the step grid as a view over the pattern's notes,
  not a second data model. It also reports the notes it *cannot* show.
- `app/commands/PatternCommands` — add / duplicate / delete / rename pattern,
  pattern and per-channel length, swing, add / delete / rename channel, channel
  volume, pan, mute, solo, step toggle, and pattern clip placement. All
  undoable; fader and length drags merge into one undo entry.
- 23 tests covering the Phase 8 exit criterion, channel ownership, polymetry,
  swing, step editing, command identity across undo/redo, the compiled graph,
  and the Piano Roll's channel filter.
- `ui/macos/ChannelRackView` — the channel rack: channels with mute, solo and
  a step grid, drawn with Core Graphics. Clicking a step is a command; a
  channel that loops shorter than the pattern greys out the steps it never
  reaches, and a channel with notes off the step grid says how many.
- Song / pattern mode (⌘M). Pattern mode loops the pattern being edited; song
  mode plays the clips on the timeline.
- Ghost notes: the Piano Roll edits the selected channel and draws the rest
  dimmed. They cannot be clicked or box-selected, and selecting a channel
  whose notes are off-screen scrolls to them.

**Changed**

- The app no longer builds its own render graph. `main.mm` opens a project with
  three channels and a starter pattern, and everything it plays comes from
  `compileProjectGraph`.
- Project format 1.1: notes gained `channelId`; patterns gained `stepDivision`,
  `swing` and `channelSettings`. Backward-compatible — a 1.0 package loads
  through an empty migration and means exactly what it meant before.
- `plugins::Format` gained `builtin`, for INCDAW's own instruments. A new
  channel is created with the built-in synth, so it makes a sound immediately.
- `compilePattern` gained a `bounded` option. A pattern's length is a loop
  marker, not a guillotine; a clip's span is a boundary, and content is confined
  to it.

**Fixed**

- `ChannelStripNode` commits its smoother state once per block rather than per
  channel, so a buffer with more than two channels ramps identically in all of
  them.


### Phase 0 — Research and architecture — 2026-08-14

**Added**

- Git repository initialised on branch `main`, with `.gitignore`.
- `docs/ARCHITECTURE.md` — layer model, threading model, data model, command
  architecture, engine boundary.
- `docs/DECISIONS.md` — decision log D-001…D-010.
- `docs/ROADMAP.md` — phases 0–20 with testable exit criteria.
- `docs/REQUIREMENTS.md` — functional scope and FL Studio 2026 reference notes.
- `docs/AUDIO_ENGINE.md` — device layer, realtime scheduling, PDC, offline
  render, correctness requirements.
- `docs/PLUGIN_HOST.md` — format support, isolation strategy, parameter and
  state systems.
- `docs/PROJECT_FORMAT.md` — `.incdaw` v1.0 package format, versioning,
  migration, media handling.
- `docs/TESTING.md` — test levels, specialised tests, stated coverage gaps.
- `docs/PERFORMANCE.md` — targets, instrumentation, method.

**Decided** (see docs/DECISIONS.md)

- C++20 core, CMake + Ninja build, direct CoreAudio/CoreMIDI, `os_workgroup`
  realtime scheduling, AppKit + Metal UI.
- Plugin formats: CLAP → AU → VST3. VST2 excluded (SDK not licensable).
- Closed-source; permissive dependencies only. JUCE rejected on licensing.
- macOS first; Windows a later target behind `platform/` isolation.
- Distribution as an ad-hoc-signed, un-notarized `.dmg`.

### Phase 1 — Foundation and build system — 2026-08-14

CMake 3.28+/Ninja build, six layer libraries, warnings as errors, doctest
harness, `tools/check_layering.py` wired into ctest, and `tools/make-dmg.sh`
producing a verified, ad-hoc-signed DMG.

### Phase 2 — Audio engine foundation — 2026-08-14

CoreAudio HAL device layer, realtime-safety guard, lock-free SPSC queue,
denormal control, audio buffer pool and views, render graph with topological
sort and cycle rejection, atomic graph swap with deferred reclamation, sine
and gain nodes, callback profiler, `tools/audiocheck`.

Verified: 440 Hz through real hardware, 0 overruns, 0 realtime allocations.

### Phase 3 — Transport — 2026-08-14

Tempo map with precomputed segment frames, time-signature map, transport
state machine, block-splitting plan for loop wraps, metronome.

Verified: click events land on the tempo map's exact frame across tempo
changes, loop boundaries, and every block size from 32 to 1024.

### Phase 4 — Project model and format — 2026-08-14

Deterministic ordered JSON, the full entity model (§24-compliant, nothing
collapsed), `.incdaw` package v1.0 with staged writes, migration hook, and a
hand-written permanent v1.0 fixture.

Verified: round-trip equality, byte-identical repeat saves, corrupt-pattern
containment, newer-format refusal, missing-media survival.

**Current state**

123 test cases, 29.5k assertions passing. Phases 5-20 not started.
