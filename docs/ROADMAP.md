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

**8a — model and compilation: COMPLETE (2026-08-14).** Per-channel pattern
content, swing, polymetric channel lengths, arrangement compilation, and
`project::compileProjectGraph` — the Project → render graph seam. Exit criterion
covered by "one pattern placed several times plays identically at each
placement" in `tests/unit/PatternTests.cpp`.

**8b — UI: COMPLETE (2026-08-14).** Channel Rack with mute, solo, volume and a
step grid; pattern list with add, duplicate, rename and remove; every edit a
command on the shared undo stack. A step is an ordinary note (D-016), so the
rack and the Piano Roll edit the same objects.

Automation inside patterns is Phase 11; `Pattern::automationLanes` serializes
but nothing evaluates it. Per-step probability and per-step parameters are
reachable through the Piano Roll — they are note properties — but the rack has
no UI for them yet.

---

## Phase 9 — Playlist / arrangement

Audio, pattern, and automation clips; free placement, split, resize, stretch,
fade, crossfade, **clip gain / pan / normalize**, grouping, locking, colouring;
track folders and lanes; markers and regions.

**Exit criterion:** a full arrangement plays back sample-accurately; clip gain
and normalize are applied pre-mixer and are recallable from the project file.

**9a — pattern arrangement: COMPLETE (2026-08-14).** Tracks, pattern clips,
placement, drag, resize, duplicate, mute, box selection and snap; a Pattern/Song
transport mode (D-017); track mute and solo resolved when the arrangement
compiles (D-018). The first half of the exit criterion — a full arrangement
playing back sample-accurately — is tested through the compiled graph in
`tests/unit/PlaylistTests.cpp`.

**9b — audio clips: COMPLETE (2026-08-15).** Audio clips play through
`AudioClipNode` (preloaded or streamed, Phase 12 parts 3-4), are visible in
the playlist with their waveform drawn in the clip body, move and resize on
the grid with tempo-aware frame math and snapshot-exact undo
(`tests/unit/AudioClipEditingTests.cpp`), and clip gain, normalize and
linear fades are applied pre-mixer and recalled from the project file —
**the exit criterion's second half, asserted in
`tests/unit/RecordingPlacementTests.cpp`**. Phase 9 is COMPLETE.

Deferred, deliberately: split/stretch/crossfades and clip pan/reverse (with
the time-stretch subsystem, Phase 16-adjacent), normalize on STREAMED clips
(a full file pass per rebuild; needs a cached peak), track folders, lanes,
markers and regions, clip grouping/locking. Automation clips wait on 11b.

---

## Phase 10 — Mixer and routing

Insert chains, sends, returns, buses, subgroups, master; arbitrary DAG routing
with cycle detection; sidechain routing; metering (peak, RMS, LUFS-ready);
**plugin delay compensation**.

**Exit criterion:** PDC test — a chain with artificial latency stays
phase-aligned with an uncompensated parallel path.

**COMPLETE (2026-08-14).** Delay compensation in `GraphBuilder` (D-019), mixer
strips with fader, constant-power pan, mute, solo, polarity and metering (D-020,
D-021), channels routed to mixer nodes, sends as gain-carrying edges, buses,
cycle detection, and a mixer pane whose meters read the live graph. Exit
criterion in `tests/unit/MixerTests.cpp`, which also asserts that the same test
fails with compensation disabled.

Outstanding within this phase's scope, and deliberately so: **insert chains** are
empty because there is nothing to insert until Phase 13 (plugins) and Phase 15
(built-in DSP); **sidechain** edges serialize and compile to nothing, because a
sidechain has no destination until a plugin can receive one; **pre-fader sends**
are recorded in the model and currently behave as post-fader. LUFS metering is
architecturally ready (`LevelMeter` integrates over a window) but not
implemented — that is Phase 15's loudness meter.

---

## Phase 11 — Automation

One generic automation subsystem serving every automatable parameter: points,
curves, ramps, step transitions, tension, clip-based and lane-based automation,
recording modes, copy/paste, scaling.

**Exit criterion:** any parameter registered in the parameter system is
automatable with no parameter-specific code.

**11a — the subsystem: COMPLETE (2026-08-15).** `ParameterRegistry` +
`AutomationSequence` + `AutomationNode` (D-022): points, linear/hold/smooth/
exponential segments, tension, per-block evaluation through the mixer's
smoothed setters, offline-render equivalence by construction. Exit criterion
tested by registering an unknown key and automating it through the compiled
graph. Commands: lane add/remove, wholesale point edits, merged drags.

**11b — placement and recording: COMPLETE (2026-08-15).** Automation clips
play (a windowed engine binding per placement, D-026 — nothing before the
clip, hold after it), `Pattern::automationLanes` is finally evaluated
(pattern clips carry their lanes, windowed the same way), automation clips
are visible/movable/resizable in the playlist with their envelope drawn in
the clip body, and write-mode recording works end to end: arm Write
Automation, ride mixer faders while the transport rolls, and the pass lands
as thinned lane points — a new lane arrives with a clip and a track, one
undo, same ids on redo (`tests/unit/AutomationClipTests.cpp`). Phase 11 is
COMPLETE.

Deferred, deliberately: touch/latch recording modes and loop-aware overdub
(D-026 tradeoffs), a dedicated point-editing surface (points are edited
through commands today), copy/paste and scaling UI.

---

## Phase 12 — Recording and audio editor

Multi-input recording, monitoring, latency compensation, loop recording, take
management, punch recording, pre-record buffer, and the **Audio Logger**
(60-second master ring buffer). Edison-style editor: waveform view, trim, fade,
normalize, gain, reverse, silence, markers, regions.

**Exit criterion:** measured loopback test — recorded audio lands
sample-accurately against the source, proving reported device latency is
correctly applied.

**STARTED (2026-08-15).** Part 1: the WAV codec (`WavFile`). Part 2: input
capture through a second IOProc on the input device (D-023), the streaming
WAV writer, `SampleRingBuffer`, and `AudioRecorder` — capture is wait-free,
drops are counted honestly, and the take's start is reported with the
device's total input latency subtracted. **The exit criterion is met and
asserted deterministically** in `tests/unit/AudioRecorderTests.cpp`: a
simulated loopback lands sample-accurately with compensation applied and
exactly `latency` frames late with it removed; `incdaw-audiocheck --record`
verifies the same path on real hardware (0 drops, 0 overruns, 0 realtime
allocations, two independently-clocked devices).

Part 3: recording lands in the timeline. The engine publishes a per-block
host-time/timeline correlation (`TimelineAnchor`, D-024);
`RecordingSession` maps the take through it; `InsertRecordedTakeCommand`
lands asset + clip undoably; `AudioClipNode` makes audio clips audible in
the arrangement; the app records on `R`, opening the input only when asked.
Verified on hardware against a rolling transport.

Part 4: the disk streamer. `WavStreamReader` decodes any frame range without
loading the file; `AudioStream` is a double-buffered window under seqlocks —
wait-free realtime reads, starvation served as counted silence (D-025);
`DiskStreamer` services every live stream from one thread. Assets over a
threshold stream, one window per clip, prefilled at compile time; streamed
playback is proven bit-identical to preloaded playback through the compiled
graph.

Part 5: the audio editor. Pure region operations (`AudioEdits`), waveform
overviews through the streaming reader (`WaveformOverview`), destructive
edits that undo bit-exactly (`AudioEditCommands` — snapshots, and redo
writes the recorded result), and the editor pane itself: waveform,
selection, zoom; opened by double-clicking an audio clip; verbs in the
Audio menu; every edit immediately audible via a graph rebuild.

Part 6: input monitoring. The engine's monitor ring bridges the input and
output clock domains; `InputMonitorNode` drains it into the master with a
drift cap and underrun-as-silence. Toggled from the Audio menu; the input
opens on demand, exactly like record arming.

Part 7: loop and punch recording. The capture is one continuous file;
placement cuts it per loop pass (stacked takes, newest audible) with the
fold resolved against the arm-time playhead, guarded by a transport seek
counter — any mid-take interference falls back to straight placement.
Punch trims placements, never the capture.

Part 8: the Audio Logger — the master's last minute as a keep-newest
circle, grabbed from the Audio menu and landed as a clip ending at the
playhead. Off by default; what was heard is exactly what comes back.

Outstanding within this phase: editor polish (markers, regions,
cut/copy/paste) and an input-side pre-record buffer (deliberately separate
from the master logger).

---

## Phase 13 — Plugin hosting

CLAP → AU → VST3, per docs/PLUGIN_HOST.md. Out-of-process scanner, registry,
blacklist, parameter and state bridges, editor hosting, latency reporting,
crash isolation.

**Exit criterion:** real third-party plugins load and process; the deliberate
misbehaviour matrix passes — **a crashing plugin does not take down INCDAW and
does not lose the project.**

**STARTED (2026-08-15).** Part 1: the CLAP foundation — vendored CLAP 1.2.6
(D-027), `SharedLibrary` behind the platform boundary, `ClapLibrary` /
`ClapInstance` (load, enumerate, activate, process), the out-of-process
`incdaw-pluginscan` with crash classification, and the test suite's own
well-behaved and hostile CLAP plugins. The misbehaviour matrix's first row
passes: a plugin that segfaults on load kills the scanner, and the host
finishes the test. Outstanding: registry + blacklist persistence, the graph
node, parameter/state/editor bridges, AU and VST3.

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

# UI build-out (post-0.9)

Phases 0–20 built the core. What follows are the U-phases: FL-Studio-class
workspaces over the finished engine, binding UI to surfaces that already
exist (CompiledProjectGraph handles, the command registry, ParameterRegistry,
the offline renderer, the sample cache). U-phases follow the same rules as
engine phases: one coherent increment, a testable exit criterion, headless
tests for everything that does not strictly need a window.

## Phase U1 — Project safety

Dirty-state tracking (save points in the command registry), a save prompt on
quit, timed sidecar autosaves with crash recovery on the next launch, and an
Open Recent menu.

**Exit criterion:** killing the app with unsaved edits offers the autosave on
the next launch; undoing back to the save point reads as clean, a merged
gesture after it reads as dirty — asserted headlessly.

## Phase U2 — Generic parameter panel

Every parameter of a builtin effect, builtin instrument, or hosted plugin
without its own editor is visible and editable from a generic panel driven by
discovered `PluginParameterInfo` / registered `ParameterRegistry` keys.
Edits are undoable commands and follow the same appliers automation uses.

**Exit criterion:** a builtin effect parameter changed from the panel equals
the same change made through its registry applier, and undoes in one step.

## Phase U3 — Export options dialog

The render library's options reach the user: master/stems/tracks/regions,
sample rate, bit depth, dither, normalize, tail. The menu today renders
master/float32/WAV only.

**Exit criterion:** a stems export from the dialog produces the same files the
library call produces headlessly.

## Phase U4 — MIDI mapping list and channel-rack learn

A window listing every mapping (key, target, range; remove/re-learn), and
learn armable from the channel rack, not only the mixer menu.

**Exit criterion:** a mapping created by learn appears in the list, and
removing it there stops the CC from moving the parameter.

## Phase U5 — Sampler zone editor

A zone-mapping editor over `Channel::samplerZones`: key/velocity ranges,
root, slice, loop points, per-zone audition. Filter/LFO parameter surface
(engine setters exist since Phase 14; no UI reaches them).

**Exit criterion:** a zone edited in the editor plays with the edited mapping
after save/load.

## Phase U6 — Browser

Samples, presets, projects; folders, favorites, search, preview,
drag-and-drop into channels and the playlist.

**Exit criterion:** a sample dragged from the browser onto a channel loads as
that channel's sampler program, undoably.

## Phase U7 — Workspace

Dockable panels, layout persistence, a shortcuts editor over the command
registry, theme polish.

**Exit criterion:** a rearranged layout survives relaunch.

---

## Deliberately out of scope

- **VST2 hosting** — not licensable (D-007)
- **Notarized public distribution** — no Developer ID by decision (D-009)
- **Windows builds** — future target; `platform/` isolation keeps it possible (D-005)
- **Cloud/collaboration features**
- **Bundled commercial sample content** — original or public-domain assets only
