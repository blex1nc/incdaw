# INCDAW — Changelog

All notable changes to this project are recorded here.
Format loosely follows Keep a Changelog. The project is pre-release; there is no
public version yet.

---

## [Unreleased]

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
  still opens one pattern on one channel.
- No `tests/fixtures/v1.1/` fixture yet — required before 1.1 ships.

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
