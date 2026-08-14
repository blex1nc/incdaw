# INCDAW — Roadmap

Status: **Phase 0 complete (documentation). Phase 1 not started.**

Phases follow CLAUDE.md §37. Each phase has a **testable exit criterion** — a
phase is not complete because code exists or compiles, but because its criterion
passes (CLAUDE.md §44). A phase that has not met its criterion does not advance.

---

## Phase 0 — Research and architecture ✅ COMPLETE

Repository, environment, architecture and FL Studio 2026 functional-reference
discovery. Stack decisions recorded in docs/DECISIONS.md (D-001…D-010).
Documentation set created. Git initialised.

**Exit criterion:** stack approved by the user. — **Met 2026-08-14.**

---

## Phase 1 — Foundation and build system

- Install CMake and Ninja *(pending dependency approval)*
- `CMakeLists.txt`, layer skeleton under `src/`, warnings-as-errors
- Test runner (doctest) and `tests/` structure
- **Layering test**: fails the build if `engine/` references `ui/`, or if macOS
  symbols appear outside `platform/`
- `tools/make-dmg.sh`: `.app` bundle → ad-hoc `codesign` → `hdiutil` DMG

**Exit criterion:** `cmake --build` is green with zero warnings; the layering
test runs in CI; the DMG mounts and the (empty) app launches on this machine.

> Note: the DMG pipeline is built in Phase 1, not Phase 20, so that packaging is
> exercised continuously and never becomes a late surprise.

---

## Phase 2 — Audio engine foundation

- CoreAudio device layer: enumeration, open/close, separate in/out devices,
  hot-plug, sample-rate and buffer-size changes
- Realtime thread joined to the device `os_workgroup`
- Lock-free SPSC queues (UI↔audio) and the background reaper
- Realtime-safety guard (allocation/lock detector) armed in debug builds
- `RenderGraph`: compile, topological sort, cycle detection, buffer allocation
- Callback-duration histogram and CPU meter

**Exit criterion:** a sine wave reaches the output; **zero underruns over a
60-minute soak**; the realtime guard reports zero allocations and zero locks on
the audio thread.

---

## Phase 3 — Transport

- Play, stop, pause, record-arm, loop
- Tempo map and time-signature map
- Sample-accurate position; seek during playback
- Block splitting at event boundaries, loop wraps and tempo changes
- Metronome, count-in, pre/post-roll, punch in/out

**Exit criterion:** an automated test proves click-track events land
sample-accurately across tempo changes and loop boundaries.

---

## Phase 4 — Project model and format

- Full data model per docs/ARCHITECTURE.md §5, stable 64-bit ids
- `.incdaw` v1.0 package: save, load, autosave, recovery
- Migration framework and the first version fixture
- Missing-media detection and relinking

**Exit criterion:** round-trip test (save → load → identical model) and
determinism test (save → save → byte-identical) both pass; the v1.0 fixture
loads.

---

## Phase 5 — MIDI engine

- CoreMIDI input/output, timestamped, UMP/MIDI-2.0-capable
- Recording with tempo-map-aware conversion and minimal jitter
- Quantize, humanize, MIDI routing and thru
- MPE-ready event representation
- Per-note extensible property slot (needed for note labels, probability, etc.)

**Exit criterion:** recorded MIDI reproduces input timing within a defined
tolerance across a tempo change.

---

## Phase 6 — Piano Roll

Metal-rendered editing surface: create, delete, move, resize, duplicate,
multi-select, box/lasso select, velocity editing, quantize, scale snapping,
ghost notes, CC/pitch-bend/modulation lanes, zoom and scroll, full undo.

**Exit criterion:** 60 fps sustained with **10,000 notes**; every edit
undoable; no direct model mutation outside the command registry.

---

## Phase 7 — Channel and instrument system

Channel model, instrument API, and one reference built-in instrument to prove
the API. Mute, solo, volume, pan, routing, preset state, colour, grouping.

**Exit criterion:** a MIDI note played into a channel produces audible sound
through the graph, with correct routing to the mixer.

---

## Phase 8 — Pattern system

Reusable, independently editable patterns containing MIDI and automation.
Step sequencer: velocity, probability, per-step parameters, swing, pattern
length, polymetric lengths.

**Exit criterion:** one pattern placed multiple times in the arrangement plays
identically at each placement; editing it updates all placements.

**Status: MET (2026-08-14).** `tests/unit/PatternTests.cpp`, "one pattern placed
twice plays identically at both placements". Velocity, probability, swing,
pattern length and polymetric per-channel lengths are implemented and tested;
per-step parameters beyond velocity and probability are not.

---

## Phase 9 — Playlist / arrangement

Audio, pattern, and automation clips; free placement, split, resize, stretch,
fade, crossfade, **clip gain / pan / normalize**, grouping, locking, colouring;
track folders and lanes; markers and regions.

**Exit criterion:** a full arrangement plays back sample-accurately; clip gain
and normalize are applied pre-mixer and are recallable from the project file.

**Status: PARTIALLY MET (2026-08-14).** `tests/unit/PlaylistTests.cpp`, "a full
arrangement plays back sample-accurately" — three bars of pattern clips with a
silent gap, rendered through the compiled graph, onsets landing within a few
frames of each clip's start and nothing before it. Clip gain applies pre-mixer
and survives a save (D-014); `normalize` serializes but has nothing to
normalize yet.

Pattern clips, free placement, split, resize, duplicate, move between tracks,
mute, lock, colour, track folders with inherited mute (D-015), and box selection
are implemented and tested. **Not implemented:** audio clips, stretching, fades
and crossfades, markers and regions, clip grouping, track lanes. All four depend
on audio clips, which need a file reader and a sample-playing node — Phases 12
and 14.

---

## Phase 10 — Mixer and routing

Insert chains, sends, returns, buses, subgroups, master; arbitrary DAG routing
with cycle detection; sidechain routing; metering (peak, RMS, LUFS-ready);
**plugin delay compensation**.

**Exit criterion:** PDC test — a chain with artificial latency stays
phase-aligned with an uncompensated parallel path.

---

## Phase 11 — Automation

One generic automation subsystem serving every automatable parameter: points,
curves, ramps, step transitions, tension, clip-based and lane-based automation,
recording modes, copy/paste, scaling.

**Exit criterion:** any parameter registered in the parameter system is
automatable with no parameter-specific code.

---

## Phase 12 — Recording and audio editor

Multi-input recording, monitoring, latency compensation, loop recording, take
management, punch recording, pre-record buffer, and the **Audio Logger**
(60-second master ring buffer). Edison-style editor: waveform view, trim, fade,
normalize, gain, reverse, silence, markers, regions.

**Exit criterion:** measured loopback test — recorded audio lands
sample-accurately against the source, proving reported device latency is
correctly applied.

---

## Phase 13 — Plugin hosting

CLAP → AU → VST3, per docs/PLUGIN_HOST.md. Out-of-process scanner, registry,
blacklist, parameter and state bridges, editor hosting, latency reporting,
crash isolation.

**Exit criterion:** real third-party plugins load and process; the deliberate
misbehaviour matrix passes — **a crashing plugin does not take down INCDAW and
does not lose the project.**

---

## Phase 14 — Sampler

Sample loading, start/end, loop points and crossfade, root note, key and
velocity mapping, pitch, reverse, envelopes, filters, LFOs, layering,
multisampling, disk streaming.

**Exit criterion:** a multisampled instrument plays correctly across the
keyboard with velocity layers, streaming from disk without underruns.

---

## Phase 15 — Built-in DSP

Shared effect interface plus an initial suite: EQ, compressor, limiter, gate,
saturation, delay, reverb, filters, utility, analyzer.

**Exit criterion:** each effect passes a null test against its own reference
implementation; all share one interface with no special-casing.

---

## Phase 16 — MIDI hardware and controller linking

Controller input, generic parameter mapping, MIDI learn, custom mappings,
hardware feedback, MIDI clock and sync.

**Exit criterion:** a hardware control mapped by learn mode drives a mixer,
instrument, and plugin parameter, and the mapping survives save/load.

---

## Phase 17 — Rendering and export

Master render, stems, individual tracks, selected regions; tail handling,
normalization, sample-rate conversion, bit depth, dither. WAV, AIFF, FLAC;
Standard MIDI File import/export.

**Exit criterion:** **offline render is byte-identical to a realtime capture**
of the same project.

---

## Phase 18 — Performance

Profiling of audio callback, UI frame time, project load, waveform and editor
rendering. Optimisation only where measurement justifies it (CLAUDE.md §27).

**Exit criterion:** every optimisation has a documented before/after
measurement.

> Instruments requires full Xcode, which is not installed. If GPU or system
> profiling proves necessary here, that becomes a dependency decision at the
> time — it is not needed before Phase 18.

---

## Phase 19 — QA

Full regression suite, fuzzing of project loading and plugin scanning, plugin
crash matrix, long-session stability, large-project stress.

**Exit criterion:** the whole suite green; no known crash-class defects open.

---

## Phase 20 — Release engineering

Versioning, release notes, ad-hoc-signed DMG, first-launch instructions for
team members (Gatekeeper quarantine — see D-009), update process.

**Exit criterion:** a team member installs from the DMG on a different Mac and
launches INCDAW successfully by the documented procedure.

---

## Deliberately out of scope

- **VST2 hosting** — not licensable (D-007)
- **Notarized public distribution** — no Developer ID by decision (D-009)
- **Windows builds** — future target; `platform/` isolation keeps it possible (D-005)
- **Cloud/collaboration features**
- **Bundled commercial sample content** — original or public-domain assets only
