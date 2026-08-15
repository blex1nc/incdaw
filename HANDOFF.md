# INCDAW — HANDOFF

Version: 2.2
Status: PHASES 0-11 COMPLETE / PHASE 12 FUNCTIONALLY DONE / PHASE 13 PARTS 1-4 DONE
Last updated: 2026-08-15
Project: INCDAW
Reference DAW: FL Studio 2026
Primary coding agent: Claude Code

---

# 1. PROJECT MISSION

INCDAW is intended to become a professional Digital Audio Workstation.

The long-term objective is functional parity with the major workflows and capabilities expected from FL Studio-class DAWs, while using independent code, architecture, algorithms, assets, branding, and UI.

INCDAW must eventually be capable of:

- beat production
- MIDI composition
- piano roll editing
- pattern sequencing
- audio recording
- audio editing
- playlist arrangement
- mixing
- routing
- automation
- sound design
- sampling
- plugin hosting
- mastering
- rendering
- MIDI hardware control
- professional project management

---

# 2. CRITICAL OPERATING RULE

CLAUDE MUST NOT IMPLEMENT ANYTHING WITHOUT USER APPROVAL.

The default state is:

RESEARCH → GRAPHIFY → PLAN → STOP → USER APPROVAL → IMPLEMENT → VERIFY

Claude may investigate and plan autonomously.

Claude may NOT:

- edit source files
- create implementation code
- delete files
- rename architecture
- install packages
- change dependencies
- run destructive commands
- make architectural changes

until the user explicitly approves the displayed plan.

---

# 3. GRAPHIFY

Graphify is mandatory for architectural work.

Before major work:

1. `/graphify .`
2. inspect `graphify-out/GRAPH_REPORT.md`
3. query relevant relationships
4. inspect only relevant source files
5. plan

After meaningful implementation:

1. rebuild/update graph
2. verify architecture
3. check disconnected components
4. record useful discoveries

If Graphify is unavailable, say so.

Never fake Graphify output.

---

# 4. REFERENCE PRODUCT

FL Studio 2026 is the primary functional reference.

Reference areas include:

- Playlist
- Piano Roll
- Channel Rack
- Step Sequencer
- Mixer
- routing
- automation
- MIDI
- audio recording
- audio editing
- instruments
- effects
- plugin hosting
- sampler
- project management
- rendering
- browser/content
- controller integration
- chord generation
- background audio recording
- AI-assisted workflow concepts

The reference is for functional analysis.

Do not copy proprietary source code, assets, plugins, presets, or visual identity.

---

# 5. CURRENT STATUS

Current phase:

PHASES 0-8 COMPLETE (2026-08-14)
PHASE 9 (playlist) — COMPLETE (9b done 2026-08-15; deferred items in ROADMAP)
PHASE 10 (mixer, routing, PDC) — COMPLETE
PHASE 11 (automation) — COMPLETE (11b done 2026-08-15: windowed placements
         D-026, pattern automation, automation clips in the playlist, write
         mode recording; touch/latch + point editor deferred, see ROADMAP)

The user authorised continuous execution through the phases, which supersedes
the per-phase approval gate in CLAUDE.md for this run. Each phase is still
gated on its own testable exit criterion (docs/ROADMAP.md) and committed
separately.

Build state:

  cmake -S . -B build -G Ninja && cmake --build build && (cd build && ctest)
  ./tools/make-dmg.sh          -> dist/INCDAW-0.1.0.dmg

  411 test cases, 169,335 assertions, green in both Debug and Release.
  Zero compiler warnings (-Werror is on).

  third_party/ is gitignored wholesale. A fresh clone refetches:
    doctest.h  — see "Dependencies in tree"
    clap 1.2.6 — git clone --depth 1 --branch 1.2.6 \
                   https://github.com/free-audio/clap  (copy include/clap
                   to third_party/clap/clap; LICENSE alongside; D-027)

  third_party/doctest/doctest.h is gitignored. A fresh clone must re-fetch it
  (command under "Dependencies in tree" below) before the first configure.

Implemented:

  Phase 0  documentation + decisions D-001..D-010
  Phase 1  CMake/Ninja, six layer libraries, doctest, layering test, DMG
  Phase 2  CoreAudio device, realtime guard, lock-free queue, render graph,
           AudioEngine with atomic graph swap, sine/gain nodes, profiler
  Phase 3  TempoMap, Transport, block-split plan, MetronomeNode
  Phase 4  Json (deterministic), full entity model, .incdaw v1.0, migration
           hook, permanent v1.0 fixture
  Phase 5  HostTime, CoreMIDI device, MidiMessage/MidiBuffer, MidiInput
           (host time -> frame offset), MidiRecorder, quantize/humanize

Phase 6 — COMPLETE. Done and tested:

  app/Command + app/CommandRegistry  undo/redo, merging, bounded history,
                                     action registration, command search
  app/commands/NoteCommands          add, delete, move, resize, velocity,
                                     quantize — all reversible, drags merge
  app/PianoRollModel                 viewport, culling, hit testing, box
                                     selection, snap, selection bookkeeping

  Measured: culling 10,000 notes costs 0.012 ms/frame (Release), 0.06 ms
  (Debug), against a 16.6 ms budget. Hit testing 10,000 notes: 0.02 ms.

  ui/macos/PianoRollRenderer      Metal, instanced rectangles, one draw call
  ui/macos/PianoRollView          layer-hosting NSView driven by a display
                                  link, mouse and keyboard routed to commands
  ui/macos/main.mm                window, status line, starter pattern

  The app now opens a working Piano Roll: click to add, drag to move, drag the
  right edge to resize, right-click to delete, shift-drag to box select,
  Q to quantize, Cmd+Z/Cmd+Shift+Z to undo/redo, Cmd+A to select all,
  scroll to pan, Cmd+scroll to zoom.

Phase 12 — IN PROGRESS. Done and tested so far:

  engine/audio/WavFile   RIFF/WAVE read/probe/write: PCM 16/24/32 + float32,
                         EXTENSIBLE unwrapped, chunk walking with pad bytes,
                         sign-correct 24-bit. Round trips bit-exact (float) /
                         within one step (PCM). This is the gate 9b, 11b and
                         the editor were waiting on.

  Part 2 (2026-08-15) — input capture and the recorder:

  platform/AudioDevice        captureAudioBlock (default no-op), separate input
                              device selection ("default" sentinel; empty still
                              means "never open the microphone unasked"),
                              totalInputLatencyFrames, input channel reporting
  CoreAudioDevice             second IOProc on the input device (Macs: the mic
                              is its own HAL device); duplex devices reuse the
                              main proc's input arguments; input buffer size
                              record-and-restore; rate mismatch = hard failure
                              with both rates named (D-023)
  engine/audio/WavBytes.h     one encoder shared by both WAV writers
  engine/audio/WavStreamWriter incremental writing, sizes patched on finalize;
                              byte-identical to WavFile::write (asserted);
                              unfinalized file probes as an empty take
  engine/core/SampleRingBuffer SPSC bulk sample ring, runtime capacity
  engine/audio/AudioRecorder  wait-free capture -> ring -> polling writer
                              thread -> disk; whole-frame drops counted and
                              reported in the Take; take start reported with
                              total input latency subtracted
  engine/AudioEngine          setCaptureSink (atomic, the capture twin of the
                              graph swap), input passthroughs
  audiocheck --record         hardware verification (--input UID, --take PATH)

  THE PHASE 12 EXIT CRITERION IS MET AND ASSERTED IN CI:
  tests/unit/AudioRecorderTests.cpp runs a simulated loopback — recorded audio
  lands sample-accurately with the reported latency applied, and exactly
  `latency` frames late with compensation removed (the PDC-test pattern: the
  pass is proven to depend on the mechanism). Verified on hardware with two
  independently-clocked devices (AirPods out + MacBook mic): 2 s take,
  0 dropped frames, 0 overruns, 0 realtime allocations. The AirPods HFP mic
  (24 kHz nominal) is correctly refused.

  Part 3 (2026-08-15) — recording lands in the timeline:

  engine::TimelineAnchor       every rendered block publishes (host time,
                               timeline frame) via a seqlock (D-024)
  engine/audio/AudioClipNode   audio clips are AUDIBLE: preloaded planar
                               playback at exact frames; gain, mute, linear
                               fades; pan/reverse/pitch/stretch are 9b
  project/RecordingSession     arm -> capture -> finish -> Placement; maps the
                               compensated take start through the anchor onto
                               the timeline (stopped transport: the playhead)
  project::clipStartTicks/
          clipLengthTicks      the D-013 accessor: placement resolved by clip
                               type; the playlist draws audio clips through it
  app InsertRecordedTakeCommand take -> AudioAsset -> audio clip on the first
                               audio track (created if none); one undo, redo
                               restores identical ids; undo never deletes the
                               file on disk
  ProjectGraphCompiler         audio tracks compile; assets decoded once and
                               shared; unreadable/wrong-rate assets are SILENT
                               WITH A WARNING (CompiledProjectGraph::warnings)
  ui: R toggles recording      input opens on FIRST ARM (never unasked); HFP
                               mic failure falls back to output-only with the
                               reason in the status line; ● REC while rolling

  Hardware-verified end to end: recorded from the MacBook mic while the
  transport rolled the arpeggio — 0 drops, 0 overruns, 0 rt allocations,
  placement computed against the rolling transport.

  Part 4 (2026-08-15) — the disk streamer:

  engine/audio/WavStreamReader  random-access decode; streaming chunk walk;
                                shares ONE decoder with WavFile (WavBytes.h
                                decodeSample) so they cannot disagree
  engine/audio/AudioStream      double-buffered window, two segments under
                                seqlocks; wait-free RT read; misses are
                                counted silence, never a wait; a seek is just
                                a position the next service pass moves to
  engine/audio/DiskStreamer     one thread services all live streams (weakly
                                held — streams die with their graphs);
                                serviceOnce() public for deterministic tests
  AudioClipNode                 plays preloaded OR streamed through one
                                gain-and-fade path; prepare() sizes scratch
  ProjectGraphCompiler          assets > streamingThresholdFrames (30 s
                                default) stream, ONE STREAM PER CLIP (shared
                                windows would fight, D-025), prefilled at
                                compile so rebuilds start warm; the app owns
                                the DiskStreamer

  Proven bit-exact: streamed playback == preloaded playback through the
  compiled graph, with tiny segments forcing refills mid-play. Starvation
  and seek behaviour asserted; RT read path allocation-free under the guard.

  Part 5 (2026-08-15) — the audio editor:

  engine/audio/AudioEdits      pure region verbs: gain, peak, normalize
                               (refuses silence), reverse, silence, linear
                               fades, trim; clamped half-open regions
  engine/audio/WaveformOverview min/max buckets via the streaming reader —
                               an hour of audio never goes resident
  app/commands/AudioEditCommands destructive edits with bit-exact undo:
                               region snapshots; REDO WRITES THE RECORDED
                               RESULT (re-applying gain would compound);
                               trim keeps head+tail and reassembles on undo
  ui/macos/AudioEditorView     waveform, drag-select, pan, Cmd+scroll zoom;
                               4th editor segment (⌘6); opened by
                               double-clicking an audio clip in the playlist
  Audio menu                   Trim/Normalize/Reverse/Silence/Fades/±3 dB —
                               selection or whole file (Edison convention);
                               every edit reloads the waveform and rebuilds
                               the graph, so it is immediately audible.
                               Undo/redo staleness is caught in housekeeping
                               via the registry's undoDepth.

  Edits render as float32 whatever the source format was; the sample-rate
  guard still applies. The editor holds no audio — only the overview.

  NOT started within Phase 12: input monitoring, loop/punch recording
  (per-segment anchoring, see D-024 tradeoffs), the pre-record buffer /
  Audio Logger, and editor polish (markers, regions, cut/copy/paste,
  spectral view). Audio-clip move/resize in the playlist is 9b (the commands
  skip audio clips explicitly rather than corrupting frame-anchored
  placement with tick math).

Phase 11a — COMPLETE. Done and tested:

  engine/automation/AutomationSequence  points, linear/hold/smooth/exponential,
                                        tension; envelope semantics
  engine/automation/AutomationNode      per-block evaluation inside the graph,
                                        writes through the mixer's smoothed
                                        setters, dies with the graph (D-022)
  project/ParameterRegistry             key -> strip applier; "volume" (cubic
                                        fader law) and "pan" built in. THE exit
                                        criterion: registering a key is all a
                                        parameter needs — tested with a key the
                                        codebase has never heard of
  app/commands/AutomationCommands       lane add/remove; point edits replace the
                                        whole vector (sorted-order safety);
                                        drags merge into one undo

  Offline render equivalence is by construction: the evaluator is a node in the
  same graph the offline path renders. Rendering allocates nothing (guarded).

  ALSO: fixed the system-wide crackle. CoreAudioDevice forced the SHARED
  device's buffer size to 256 and never restored it (Bluetooth cannot sustain
  that; the setting outlived the app). And the graph was compiled for the
  device's current buffer while a shared device delivers up to its maximum —
  AirPods report 15..960 — so oversized callbacks were truncated into a
  duty-cycled buzz. See "Things to be careful about".

Phase 10 — COMPLETE. Done and tested:

  engine/graph/RenderGraph      delay compensation: delay lines inserted on the
                                short edges into any summing node (D-019).
                                setDelayCompensationEnabled(false) exists so the
                                PDC test can assert its own absence fails
  engine/dsp/DelayLineNode      fixed whole-sample delay, allocation-free
  engine/dsp/MixerStripNode     summing, polarity, constant-power pan, fader,
                                mute, metering — one node, one pass (D-020)
  engine/core/Smoother          the click-free ramp, out of GainNode
  engine/core/LevelMeter        peak + RMS over 300 ms, atomic publish
  project/ProjectGraphCompiler  mixer nodes -> strips, routing -> edges, sends
                                -> gain-carrying edges, channels -> their own
                                strip. Channel pan finally applied
  app/commands/MixerCommands    nodes, volume, pan, mute, solo, polarity,
                                channel routing, connect/disconnect, send level
  ui/macos/MixerView            strips, cubic fader law, live meters. Fader and
                                pan write straight to the rendering strip rather
                                than recompiling — the path automation will use

  Exit criterion: "a latent path stays phase-aligned with an uncompensated
  parallel path" in tests/unit/MixerTests.cpp. The same test rebuilds with
  compensation off and asserts the impulse smears into two.

  Measured (Release): 64-strip mixer 0.042 ms per 256-frame block, against a
  5.33 ms budget. Rendering allocates nothing.

  Verified on the running app: the mixer meters a playing pattern, RMS body and
  peak line both moving.

  Deliberately outstanding inside Phase 10: insert chains are empty (nothing to
  insert until Phase 13/15), sidechain edges compile to nothing, pre-fader sends
  behave as post-fader, LUFS is ready architecturally but not implemented.

Phase 9a — COMPLETE. Done and tested:

  app/commands/TrackCommands    add, remove (takes the track's clips with it),
                                rename, mute, solo, height
  app/commands/ClipCommands     add, remove, move, resize, duplicate, mute —
                                addressed by id, because every track's clips
                                share one vector
  app/PlaylistModel             culling, hit testing, resize handles, box
                                selection, snap — headless
  ui/macos/PlaylistView         ruler, track headers, clips, drag/resize/paint,
                                ruler seeking, playhead
  project/PatternCompiler       track mute and solo resolved at compile time
                                (D-018)

  Pattern/Song transport mode (D-017): the mode picks PlaybackSource when the
  graph is compiled, so the audio thread never learns there are two modes.
  ⌘1/⌘2 switch editors, ⌘3/⌘4 switch modes.

  Measured (Release): recompiling an arrangement of 512 clips over 64 notes
  costs 0.088 ms — which is what makes recompiling on every mouse move during a
  drag defensible.

  Exit criterion, first half: "a pattern placed twice plays identically at both
  placements, through the graph" in tests/unit/PlaylistTests.cpp.

  NOT verified live: mouse gestures. The window was captured and inspected, but
  the machine was in use and synthetic clicks went to the frontmost application,
  so drag/resize/paint rest on the headless tests for now.

Phase 8b — COMPLETE. Done and tested:

  app/commands/ChannelCommands    add, remove, rename, mute, solo, volume
                                  (mergeable), step key
  app/commands/PatternCommands    add, duplicate, remove, rename, length, swing
  app/commands/StepCommands       ToggleStepCommand + noteAtStep. A step IS a
                                  note: there is no step data type, so the rack
                                  and the Piano Roll cannot disagree (D-016)
  app/ChannelRackModel            rack geometry and hit testing, headless
  ui/macos/ChannelRackView        channels, mute/solo, volume, step grid, drag
                                  painting, playhead column (CoreGraphics, D-015)
  ui/macos/PatternListView        select, add, duplicate, rename, remove
  project::Project                insert/remove/indexOf for channels and
                                  patterns — what undo needs to restore an
                                  entity with the identity it had

  Verified on the running app, not only in tests: adding a channel, programming
  four steps on it, muting it, undoing the mute, creating and switching
  patterns, and playing — the four steps show up in the Piano Roll as four
  notes, and the note count goes 12 -> 16.

  Project format 1.1 -> 1.2 (Channel::stepKey), additive. Fixtures for 1.0 and
  1.1 both load.

Phase 8a — COMPLETE. Done and tested:

  project/Model.h                  Pattern holds PatternChannelContent per
                                   channel, each with its own loop length
                                   (polymetry). Pattern gained swing/swingGrid.
                                   Clip gained tick placement alongside frames.
  project/PatternCompiler          per-channel compile; polymetric repeats;
                                   swing; probability; compileArrangement over
                                   the project's pattern clips
  project/ProjectGraphCompiler     Project -> CompiledGraph. One InstrumentNode
                                   + GainNode per audible channel, into a master
                                   gain. Injectable InstrumentFactory so Phase 13
                                   is a new factory, not a change here.
  app/commands/NoteCommands        every note edit addresses (pattern, channel)
  app/PianoRollModel               takes an event list, not a Pattern
  project/ProjectFile              format 1.0 -> 1.1 + migration; the 1.0
                                   fixture still loads and its notes are bound
                                   to a real channel

  Decisions recorded: D-012 (per-channel content), D-013 (tick vs frame clip
  placement), D-014 (swing applies only exactly on the grid).

  Measured (Release): recompiling 16 channels x 500 notes x 32 placements —
  256,000 note instances — costs 2.7 ms. Debug: 43.9 ms.

  Exit criterion test: "one pattern placed several times plays identically at
  each placement" in tests/unit/PatternTests.cpp, which also asserts that
  editing the pattern changes every placement.

Phase 7 — COMPLETE. Done and tested:

  engine/instrument/Instrument     API; the base class does the block-splitting
                                   so no instrument can get event timing wrong
  engine/instrument/SimpleSynth    polyphonic, PolyBLEP band-limited saw/square,
                                   ADSR, voice stealing prefers released voices
  engine/instrument/InstrumentNode merges sequenced and live MIDI; silences
                                   voices on a transport discontinuity
  engine/midi/NoteSequence         dual-sorted (by start, by end) so note-offs
                                   cost O(log n), not a scan per block
  project/PatternCompiler          Pattern -> SequencedNote; probability is
                                   rolled here, deterministically, so playback
                                   and offline render will agree

  THE APP IS NOW AUDIBLE. Space plays the pattern on loop; editing a note
  rebuilds the graph and swaps it in atomically. Verified on hardware:
  6 s arpeggio, 0 overruns, 0 realtime allocations.

WHAT THE APP STILL DOES NOT DO:

  - no automation UI: lanes evaluate and play (11a), but nothing lets the user
    draw one — no lane editor, no automation clips, no write/touch/latch
    recording. THIS IS PHASE 11b. Pattern::automationLanes still serializes
    without being evaluated.

  - no audio clips at all: there is no WAV reader, no decoder and no disk
    streaming anywhere in the tree. Clip gain, normalize, fades, crossfades,
    stretch and reverse are audio-clip properties and are therefore all
    outstanding. THIS IS PHASE 9b, and it is why Phase 9 is not complete.

  - patterns can carry automation lanes in the model; nothing evaluates them.
  - drag-painting steps leaves one undo entry per cell, not one per stroke.
  - channel colour and step key have commands but nothing in the UI reaches
    them; a drum channel has to be given its key in code.
  - no playlist/arrangement: one pattern, looped. Clip types serialize only.
  - no audio clips, no recording into the timeline, no plugins, no sampler.
  - the project is never saved from the UI: ProjectFile works and is tested,
    but no menu action calls it.

Not started:

  Phase 9b Audio clips (needs a reader)    Phase 15  Built-in DSP
  Phase 11b Automation UI/clips/recording  Phase 16  MIDI hardware
  Phase 10 Mixer/routing (model exists,    Phase 16  MIDI hardware
           no signal path)                 Phase 17  Render/export
  Phase 11 Automation (model exists,       Phase 18  Performance
           no evaluation)                  Phase 19  QA
  Phase 12 Recording/audio editor          Phase 20  Release
  Phase 13 Plugin hosting
  Phase 14 Sampler

Important: several Phase 4 model types (MixerNode, AutomationLane, Clip,
Channel) currently SERIALIZE but are not yet WIRED INTO THE AUDIO GRAPH. They
are data, not behaviour. Do not describe the mixer or automation as working.

Environment (verified 2026-08-14):

macOS 26.2 - Apple M5 arm64 - 10 cores (unknown P/E split at query time) - 16 GB
Apple clang 21.0.0 - macOS SDK 26.5 - Command Line Tools only (no Xcode.app)
CMake 4.4.2, Ninja 1.13.2 (installed during Phase 1)
No code-signing identity: the DMG is ad-hoc signed, not notarized (D-009)

Dependencies in tree:

  third_party/doctest/doctest.h  2.4.11, MIT  (gitignored; re-fetch with
  curl -sSL -o third_party/doctest/doctest.h \
    https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h)

Graphify:

  CURRENT FOR CODE, as of Phase 8b. `graphify . --code-only` needs no API key
  and now indexes the real source tree: 1794 nodes, 3207 edges, built from
  commit 1972c70. Refresh it after changes with:

    graphify . --code-only && graphify cluster-only .

  The documentation half is still unindexed: plain `graphify .` fails with
  "no LLM API key found" for the 15 doc files. Export GEMINI_API_KEY,
  ANTHROPIC_API_KEY or equivalent if the docs need to be in the graph too.

---

# 6. FIRST SESSION OBJECTIVE

The next Claude Code session must NOT start coding.

The next session must perform discovery.

Required sequence:

## 6.1 Verify environment

Determine:

- OS
- CPU architecture
- available compiler
- available SDK
- package manager
- CMake or equivalent build system
- Python version
- Git
- audio development libraries
- MIDI libraries
- installed SDKs
- plugin SDK availability

Do not install anything without approval.

---

## 6.2 Verify Graphify

Check whether:

`graphify`

is available.

If available:

Run:

`/graphify .`

Then inspect:

`graphify-out/GRAPH_REPORT.md`

If unavailable:

STOP and report it.

---

# 7. DISCOVERY REPORT

The first deliverable must contain:

## Repository

- current files
- existing source
- existing architecture
- existing documentation
- build system

## Environment

- operating system
- compiler
- SDK
- build tools

## Architecture

- existing modules
- dependency graph
- circular dependencies
- missing abstractions

## DAW Requirements

- audio
- MIDI
- transport
- timeline
- project model
- plugins
- instruments
- effects
- mixer
- routing
- automation
- rendering
- UI

## Risks

Identify high-risk areas.

At minimum:

- realtime audio
- plugin hosting
- latency
- project serialization
- audio threading
- UI performance
- cross-platform compatibility
- DSP correctness
- plugin crashes
- large project performance

---

# 8. PROPOSED ARCHITECTURAL LAYERS

The final architecture must be decided after discovery.

Candidate layers:

1. Platform
2. Audio Device
3. Audio Engine
4. DSP
5. MIDI
6. Transport
7. Timeline
8. Project
9. Routing
10. Mixer
11. Automation
12. Plugin Host
13. Instruments
14. Effects
15. Audio Editor
16. Rendering
17. Browser
18. UI
19. Commands
20. Persistence

The exact structure is NOT approved yet.

Do not implement this blindly.

---

# 9. CORE DATA MODEL

The project should eventually distinguish:

Project
→ Song
→ Timeline
→ Track
→ Clip
→ Pattern
→ Channel
→ Instrument
→ Plugin
→ Mixer Node
→ Automation
→ MIDI Event
→ Audio Asset
→ Routing Connection

Do not collapse all entities into one generic model.

---

# 10. AUDIO ENGINE PRIORITY

The audio engine is the foundation.

It must eventually support:

- realtime processing
- sample accurate scheduling
- low latency
- buffer management
- sample rate
- device I/O
- routing
- mixing
- plugin processing
- PDC
- offline rendering
- realtime rendering
- monitoring
- recording
- safe thread behavior

Realtime audio code must not depend on UI operations.

---

# 11. PLUGIN HOST PRIORITY

The plugin system should be architected early even if complete support comes later.

Potential targets:

- VST3
- Audio Units
- CLAP if practical

Required architecture:

Plugin Scanner
→ Plugin Registry
→ Plugin Instance
→ Parameter System
→ State System
→ UI Bridge
→ Audio Processor
→ Latency Reporting
→ Automation
→ Crash/Isolation Strategy

---

# 12. UI PRIORITY

Main workspaces eventually include:

- Toolbar
- Playlist
- Piano Roll
- Channel Rack
- Mixer
- Browser
- Plugin Editor
- Audio Editor
- Project Settings
- Audio Settings
- MIDI Settings

The UI must be decoupled from the audio engine.

---

# 13. FEATURE ROADMAP

The roadmap is proposed, not yet approved.

## PHASE 0
Research + architecture

## PHASE 1
Build foundation

## PHASE 2
Audio engine

## PHASE 3
Transport

## PHASE 4
Project model

## PHASE 5
MIDI

## PHASE 6
Piano Roll

## PHASE 7
Channel/instruments

## PHASE 8
Patterns

## PHASE 9
Playlist

## PHASE 10
Mixer/routing

## PHASE 11
Automation

## PHASE 12
Recording/audio editor

## PHASE 13
Plugin hosting

## PHASE 14
Sampler

## PHASE 15
Built-in DSP

## PHASE 16
MIDI hardware

## PHASE 17
Rendering/export

## PHASE 18
Performance

## PHASE 19
QA

## PHASE 20
Release

---

# 14. TESTING STRATEGY

Tests must exist at multiple levels.

Unit:

- DSP
- MIDI
- project serialization
- automation
- routing

Integration:

- audio engine + mixer
- MIDI + instruments
- plugins + mixer
- playlist + transport
- project load/save

End-to-end:

- create project
- add instrument
- program MIDI
- arrange
- mix
- automate
- render

Regression:

Every serious bug gets a regression test.

---

# 15. PERFORMANCE STRATEGY

Track:

- audio callback duration
- CPU
- memory
- UI frame time
- disk throughput
- project loading
- plugin processing
- Playlist rendering
- Piano Roll rendering

Performance work must be measured.

No premature optimization.

---

# 16. PROJECT FORMAT

INCDAW native project format must be:

- versioned
- migratable
- deterministic
- portable
- resilient to missing media
- able to store plugin state
- able to store automation
- able to store MIDI
- able to store routing
- able to store arrangement

Never create a project format without versioning.

---

# 17. DEVELOPMENT RULE

One coherent subsystem at a time.

Never generate a fake "complete DAW" in one pass.

Never build:

- fake waveform editors
- fake audio engines
- placeholder mixer logic presented as production
- UI buttons that do nothing
- mocked audio behavior presented as complete functionality

If something is a prototype, label it:

PROTOTYPE

If something is mocked:

MOCK

If something is production-ready:

PRODUCTION

---

# 18. FEATURE IMPLEMENTATION PROTOCOL

For every requested feature:

### 1. GRAPHIFY

Determine architectural connections.

### 2. RESEARCH

Determine expected behavior.

### 3. INSPECT

Find relevant existing implementation.

### 4. PLAN

Show:

- objective
- behavior
- architecture
- files
- APIs
- data model
- risks
- tests
- performance
- definition of done

### 5. STOP

Wait for approval.

### 6. IMPLEMENT

Only after approval.

### 7. VERIFY

Build + tests + relevant runtime verification.

### 8. GRAPHIFY UPDATE

Update knowledge graph.

### 9. HANDOFF

Update this file if project state materially changes.

---

# 19. HANDOFF RULE

At the end of a significant approved implementation, update:

HANDOFF.md

Include:

- current phase
- completed work
- current architecture
- files changed
- tests
- known bugs
- known limitations
- next recommended step
- pending decisions

Do not pretend the project is further along than it is.

---

# 20. OPEN DECISIONS

RESOLVED 2026-08-14. Full rationale in docs/DECISIONS.md (D-001 … D-010).

- programming language ......... C++20 / Apple clang            [D-001]
- build system ................. CMake + Ninja                  [D-002]
- audio backend ................ CoreAudio HAL, direct          [D-003]
- realtime scheduling .......... os_workgroup / Audio Workgroups[D-004]
- platform strategy ............ macOS first, Windows later     [D-005]
- UI framework ................. AppKit shell + Metal widgets   [D-006]
- plugin architecture .......... CLAP -> AU -> VST3, VST2 out   [D-007]
- licensing strategy ........... closed-source, permissive deps [D-008]
- distribution ................. ad-hoc signed .dmg             [D-009]
- version control .............. git                            [D-010]
- DSP architecture ............. own DSP, Accelerate where measured
- project serialization ........ .incdaw package, versioned v1.0
- rendering architecture ....... shared graph, offline == realtime
- threading model .............. RT audio / RT worker / UI / background
- state management ............. Project owns model; engine reads compiled graph
- command architecture ......... central CommandRegistry, every action a command
- undo/redo architecture ....... command stack, single mutation path
- dependency strategy .......... permissive only, per-item approval (CLAUDE.md 41)

STILL OPEN:

- Installing CMake and Ninja via Homebrew — approval not yet given.
  Phase 1 cannot start without them.
- Adopting doctest (MIT) as the test framework — approval not yet given.
- Vendoring CLAP and VST3 SDK headers — needed at Phase 13, not before.

---

# 21. DEFINITION OF SUCCESS

INCDAW should eventually feel like a real professional DAW rather than a collection of disconnected tools.

The following must work together:

MIDI
↓
Instrument
↓
Channel
↓
Mixer
↓
Effects
↓
Automation
↓
Master
↓
Render

And:

Audio Input
↓
Recording
↓
Audio Clip
↓
Playlist
↓
Mixer
↓
Effects
↓
Automation
↓
Master
↓
Render

And:

Plugin
↓
Parameter
↓
Automation
↓
Mixer
↓
Render

All systems must share the same underlying transport, timing, project state and undo architecture.

---

# 22. CURRENT HANDOFF MESSAGE

Phases 0-4 are implemented, tested and committed. The engine makes sound, keeps
sample-accurate time, and saves and loads a versioned project.

Read first:

1. docs/DECISIONS.md    — what was decided and why (D-001..D-010)
2. docs/ARCHITECTURE.md — layers, threading, data model, commands
3. docs/ROADMAP.md      — phases and their exit criteria
4. git log              — each phase is one commit with its rationale

Verify the current state before continuing:

  cmake -S . -B build -G Ninja && cmake --build build && (cd build && ctest)
  ./build/incdaw-audiocheck --list
  ./build/incdaw-audiocheck --seconds 3 --amplitude 0.05

Next step (phases 0-11 COMPLETE; Phase 12 is functionally done — capture,
recording into the timeline, streaming, the editor, monitoring, loop/punch
recording and the Audio Logger all work. What remains in it is polish):

    - editor polish: markers, regions, cut/copy/paste between files
    - input-side pre-record buffer (deliberately separate from the master
      logger — privacy: the mic must not be kept when the user believes
      only playback is)
    - deferred small items: normalize on STREAMED clips (cached peak),
      automation point-editing surface, touch/latch modes, latch-mode
      loop-aware automation overdub (D-026)

  PHASE 13 IS STARTED (user authorised continuous execution INCLUDING the
  CLAP vendoring; D-027 records the dependency). Part 1 done: ClapLibrary,
  ClapInstance, out-of-process scanner with crash classification, the test
  suite's own well-behaved + hostile CLAP plugins, isolation proven in CI.

  Phase 13 parts 2-3 are ALSO DONE: PluginRegistry with a persisted,
  reasoned blacklist and a zero-rescan cache (size+mtime); PluginNode
  processing a hosted plugin inside the render graph, bit-exact and
  allocation-free (plugins now links engine — same layer rank; process
  spawning moved to platform/ChildProcess when the layering checker
  objected, correctly).

  Phase 13 part 4 is DONE: a mixer node's inserts compile into a chain in
  front of its strip, PRE-FADER. The compiler keeps separate input and
  output indices per mixer node — signal enters at the head of the chain
  and leaves at the strip — so channels, audio tracks, sends and the input
  monitor all arrive ahead of the destination's plugins. Insert nodes come
  from an injected `GraphCompileOptions::insertFactory` returning an
  engine::Node, so project/ still includes no plugin header (D-028);
  plugins::PluginInstanceManager owns the ClapLibraries for the app's
  lifetime and the shell injects the factory on every rebuild. A bypassed
  slot is never instantiated; an unbuildable one is a pass-through plus a
  named warning. 9 tests, including one that fails if the chain is wired
  after the fader.

  Phase 13 part 5 is DONE: parameter discovery -> ParameterRegistry, and
  plugin parameters automate through the generic subsystem (D-029). The two
  design gates recorded here after part 4 were resolved exactly as written:
    (a) Entry::apply is now a variant — StripApplier or SinkApplier over the
        new pure interface engine::ParameterSink; which alternative an entry
        holds tells the compiler what the lane's target resolves to (the
        strip rendering the entity, or the sink of the insert slot the
        entity names, via Node::parameterSink()). Keys are scoped per plugin
        TYPE ("plugin:<uid>:<param-id>"); the lane's targetEntity picks the
        instance. The Phase 11 exit criterion held: registration is still
        all a parameter needs, and the exit-criterion test drives a hosted
        plugin's gain to unity through an automation lane with no
        parameter-specific code outside the registry.
    (b) ClapInstance carries the event queue: setParameter (audio thread,
        lock-free, preallocated) -> drained by its own process() into the
        block's clap_event_param_value input list at time 0. No setter ever
        calls into the plugin. Discovery snapshots CLAP_EXT_PARAMS at create
        time into format-agnostic PluginParameterInfo, cached per uid by
        PluginInstanceManager; the shell's insert factory registers the
        list into the app-owned ParameterRegistry during the same compile
        the lanes bind in.

  Phase 13 part 6 is DONE: plugin state save/load (§6, D-030).
  engine::StateIO mirrors the ParameterSink pattern (Node::stateIO());
  ClapInstance carries CLAP_EXT_STATE with stack-local stream adapters and
  a 64 MB hostile-save cap; CompiledProjectGraph::insertStateFor(slot)
  reaches a live insert's carrier (filled only on successful compile);
  project/PluginStateFiles captures blobs to plugins/insert-<id>.state
  (stage-and-rename) and restores after compile. stateFile was already in
  project.json, so NO format change. Ordering contract: capture BEFORE
  ProjectFile::save, restore AFTER compiling a loaded project. All failure
  modes are warnings, never a failed save; a missing plugin's slot and blob
  are never touched. NOTE: the shell calls neither yet — the app still has
  no save/open action (the standing "largest gap"), so state capture is
  wired and tested at library level only.

  Phase 13 part 7 is DONE: latency -> PDC. ClapInstance snapshots
  CLAP_EXT_LATENCY at creation (while activated, capped at 10 s against
  hostile reports); PluginNode::latencyFrames surfaces it into the engine's
  EXISTING delay compensation (built in Phase 10 — insertDelayCompensation
  was already there; part 7 only had to feed it). Proven against
  tests/plugins/TestLatencyPlugin.cpp, a true 64-frame delay that reports
  itself. NOT done: clap_host_latency.changed (mid-life latency change ->
  recompile) and the manual per-instance offset — both in PLUGIN_HOST §8.

  Phase 13 part 8 is DONE: plugins reach the user. PluginCommands
  (add/remove/bypass as undoable commands; the slot id survives redo, a
  removed slot returns in place WITH its stateFile), the mixer context
  menu's insert chain, File > Scan Plugins… through the bundled
  out-of-process scanner (incdaw-pluginscan is copied into
  INCDAW.app/Contents/MacOS at build). Insert reordering is NOT yet a
  command; the chain order can only be built by add order today.

  Phase 13 part 9 is DONE: instances outlive graphs (D-031).
  PluginInstanceManager keys live instances by slot id; PluginNode
  borrows. Rebuilds reuse the slot's instance (live state intact); a
  changed sample rate/block recreates it carrying the state blob; a
  changed uid starts fresh; the shell's retain pass after each swap is
  the only disposer. This was the precondition editor hosting was
  waiting for — an editor window can now outlive every graph rebuild.
  Caveat recorded in D-031: undoing a slot REMOVAL returns a fresh
  instance (only the saved blob survives the retain pass).

  Phase 13 part 10 is DONE: the editor bridge (§7). ClapInstance hosts
  embedded Cocoa editors through CLAP_EXT_GUI (strict call sequence, no
  editor left on refusal, destructor closes first); the shell owns one
  NSWindow per slot, opened from the mixer insert submenu, surviving
  rebuilds because of D-031 and closed BEFORE the retain pass disposes an
  instance. Tested headlessly against the gain plugin's recording gui.

  Phase 13 continues, in dependency order:
    - params->flush() while the engine is idle, plugin-originated changes
      (out_events -> recordable automation), clap_host_state.mark_dirty,
      clap_host_latency.changed -> recompile. These are the remaining
      host-callback halves of §5/§6/§8.
    - AU, then VST3 (D-007 order) — new format backends behind the same
      PluginInstanceManager surface.
    - a generic parameter surface over discovered PluginParameterInfo
      (UI listing every automatable parameter of a slot).
    - sample-accurate event splitting inside the block, once the
      AutomationNode itself evaluates finer than per-block

Things to be careful about:

  - third_party/ is gitignored: a fresh clone or worktree has neither
    doctest nor the CLAP headers, and the build fails on the test target
    and on plugins/ until both are fetched (HANDOFF section 5 has the
    doctest command; D-027 has CLAP's).
  - RESOLVED (2026-08-15): the File menu exists — Open/Save/Save As call
    ProjectFile and PluginStateFiles in the documented order (capture before
    save; restore after the rebuild). Still missing: dirty prompt on quit,
    autosave, recent projects. Unsaved work is still lost silently on quit;
    only SAVING became possible.
  - tests/fixtures/v1.1/ does not exist. Format 1.1 is covered only by the
    save/load round trip, which proves the code agrees with itself and nothing
    more. docs/PROJECT_FORMAT.md §2 requires a hand-written fixture before 1.1
    ships.
  - Two clip time bases now coexist (D-013). Pattern and automation clips use
    startTick/lengthTicks; audio clips use start/length. Phase 9 should add one
    accessor that resolves placement by clip type rather than letting each
    caller choose.

  - The mixer, automation and clip types serialize but have no audio path.
    Phases 10 and 11 must build that; nothing today evaluates them.
  - MIDI input works but nothing plays it: there is no instrument yet, so a
    note in reaches the recorder and the pattern, not a sound. Phase 7.
  - Sysex is not handled by the CoreMIDI reader (it breaks out of the packet
    walk). Fine until a plugin or controller needs it.
  - The realtime guard cannot see allocations the optimiser elides. Tests that
    deliberately allocate must call ::operator new directly, not use a
    new-expression, or they silently pass in Release.
  - A device shared with another process delivers whatever block size CoreAudio
    is servicing, not the one the property query reports. The scratch sizing,
    the profiler budget AND the render graph's buffers (GraphCompileOptions
    .maxBlockSize must come from AudioEngine::maxServiceableBlockSize, never
    from bufferSize) all account for this; do not reintroduce an assumption
    that they match. A graph compiled smaller than a delivered block renders
    the first part and leaves the rest silent, which the ear hears as a buzz —
    this actually happened with AirPods (range 15..960) and a 256-frame graph.
  - The buffer size is a property of the SHARED output device. CoreAudioDevice
    now records the value it found and restores it in close(); never remove
    that, or quitting INCDAW leaves every other application's audio broken.
    The default request is 512 because Bluetooth cannot sustain less. The same
    record-and-restore contract now applies to the input device's buffer size.
  - Capture is a second clock domain (D-023). Input timestamps are the HAL's,
    with reported input latency subtracted only at the reporting edge (the
    recorder's Take). Do not subtract it anywhere else, or it will be applied
    twice. Two-device drift within a take is real, second-order, and
    deliberately unhandled for now — it is written down in D-023.
  - There is NO sample-rate conversion on capture. An input that cannot run at
    the output's granted rate is refused with both rates named. This is hit in
    practice by Bluetooth HFP microphones (AirPods mic: 24 kHz); users must
    pick a real microphone, and the UI will eventually need to say so.
  - An unfinalized take (crash mid-recording) probes as ZERO frames by design:
    the header is written with zero sizes and patched on finalize. A recovery
    pass that reconstructs the length from the file size is possible future
    work; do not "fix" the zero header into a lie instead.

---

# 23. FINAL RULE

If there is uncertainty:

DO NOT GUESS.

Research.

If architecture is unclear:

DO NOT IMPLEMENT.

Plan.

If scope expands:

STOP.

Ask.

If Graphify is available:

USE IT.

If the user has not approved the plan:

DO NOT MODIFY THE CODEBASE.
