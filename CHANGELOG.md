# INCDAW — Changelog

All notable changes to this project are recorded here.
Format loosely follows Keep a Changelog. The project is pre-release; there is no
public version yet.

---

## [Unreleased]

### Phase 9a — Playlist: the pattern arrangement — 2026-08-14

**Added**

- `app/commands/TrackCommands` — add, remove (with the track's clips), rename,
  mute, solo, height.
- `app/commands/ClipCommands` — add, remove, move (mergeable), resize
  (mergeable), duplicate, mute. Clips are addressed by id, not by index.
- `app/PlaylistModel` — culling, hit testing, resize handles, box selection and
  snap, headless.
- `ui/macos/PlaylistView` — ruler, track headers, clips, drag/resize/paint,
  ruler seeking, playhead.
- A Pattern/Song transport mode, and a View menu binding the editors to ⌘1/⌘2
  and the modes to ⌘3/⌘4.
- `Project::insertTrack` / `insertClip` / `removeTrack` / `removeClip` /
  `indexOfTrack` / `indexOfClip` / `findClip`.
- `tests/unit/PlaylistTests.cpp` — 13 cases, including the roadmap exit
  criterion through the compiled graph, and a recompile measurement.

**Changed**

- `project::compileArrangement` honours track mute and solo (D-018).
- A new project opens with one track and the pattern placed on it, so song mode
  plays something.
- Editor panes now resize with the window; they previously kept their initial
  size and left a dead margin down the right edge.

**Known gaps**

- No audio clips, and therefore no clip gain or normalize: Phase 9b, which needs
  a file reader and a streamer. **Phase 9 is not complete.**
- No automation clips (Phase 11), no fades, crossfades, stretch or reverse.
- Live mouse interaction was not verified this session; the headless tests cover
  the edits.

### Phase 8b — Channel Rack, pattern list, step sequencer — 2026-08-14

**Added**

- `app/commands/ChannelCommands` — add, remove, rename, mute, solo, volume
  (mergeable), step key. Removing a channel takes its content in every pattern
  with it and gives all of it back.
- `app/commands/PatternCommands` — add, duplicate, remove, rename, length,
  swing.
- `app/commands/StepCommands` — `ToggleStepCommand` and `noteAtStep`. A step is
  an ordinary note (D-016).
- `app/ChannelRackModel` — rack geometry and hit testing, drawn by nobody and
  therefore tested headlessly.
- `ui/macos/ChannelRackView` — channels, mute/solo, volume, and the step grid,
  with drag-painting and a playhead column (D-015).
- `ui/macos/PatternListView` — select, add, duplicate, rename, remove.
- `Channel::stepKey` — the pitch a channel's steps are written at.
- `Project::insertChannel` / `insertPattern` / `removeChannel` / `removePattern`
  / `indexOfChannel` / `indexOfPattern` — what undo needs to put an entity back
  where it was, with the identity it had.
- `tests/unit/ChannelRackTests.cpp` — 13 cases: command round trips, mute and
  solo reaching the compiled graph, steps visible to the Piano Roll, rack hit
  testing.
- `tests/fixtures/v1.1/` — the 1.1 fixture, now that 1.1 is no longer current.

**Changed**

- Project format 1.1 → 1.2, additive: a 1.1 file has no `stepKey` and reads back
  as middle C. Both fixtures load.
- The transport loops the selected pattern rather than `patterns()[0]`.
- Retargeting the Piano Roll clears its selection, since note indices belong to
  one channel's list in one pattern.
- Space and ⌘Z work from any pane.

**Known gaps**

- Drag-painting steps leaves one undo entry per cell rather than one per stroke.
- The project is still never saved from the UI.
- Channel colour and step key have commands but no UI to reach them.

### Phase 8a — Pattern system: model and compilation — 2026-08-14

**Added**

- `project::PatternChannelContent` — a pattern now holds its notes per channel,
  with an optional per-channel loop length for polymetric patterns (D-012).
- `Pattern::swing` / `swingGrid` — shuffle resolved at compile time, applied to
  notes exactly on an odd grid line (D-014).
- `Clip::startTick` / `lengthTicks` / `sourceOffsetTicks` — musical placement,
  authoritative for pattern and automation clips (D-013).
- `project::compileArrangement` — every note a channel plays across the
  arrangement's pattern clips. A clip shorter than its pattern trims it, a
  longer one repeats it, and notes are cut at the clip boundary.
- `project::compileProjectGraph` — the Project → render graph seam, with an
  injectable `InstrumentFactory`, per-channel gain, and project-wide solo.
- `tests/unit/PatternTests.cpp` — 15 cases including the Phase 8 exit criterion
  and a recompile-cost measurement.

**Changed**

- Project format 1.0 → 1.1. Pattern files store `channels[]`; clips store tick
  placement. 1.0 files are migrated on load and the 1.0 fixture still passes.
- Note commands address a (pattern, channel) pair; `PianoRollModel` and
  `MidiCapture` take an event list rather than a `Pattern`.
- `ui/macos/main.mm` no longer assembles the render graph by hand.

**Known gaps**

- No Channel Rack, pattern list, or step sequencer UI yet (Phase 8b): the app
  still opens one pattern on one channel. *(Closed by Phase 8b.)*
- No `tests/fixtures/v1.1/` fixture yet — required before 1.1 ships.
  *(Closed by Phase 8b.)*

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
