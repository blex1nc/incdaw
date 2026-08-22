# INCDAW — HANDOFF

Version: 3.8
Status: ALL PHASES 0-20 COMPLETE, v0.9.0 — the three parallel work lines are
        merged: UI build-out increments 1-11, the FL2026 feature wave, and
        the drawn design language. Increment 11 made the machine
        configurable, connected MIDI hardware, and gave every command a
        name a keystroke can find.
Last updated: 2026-08-22
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

  637 test cases, 3,207,384 assertions, green in both Debug and Release.
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

## 2026-08-22 — the repository has a remote, and INCDAW knows when it is old

### The repository is on GitHub

`origin` = `git@github.com:blex1nc/incdaw.git`, over SSH. It is **public**.
The repository existed and was empty; `main` and all fifteen branches
(thirteen `claude/*` worktree lines, `phase-21-workspace`,
`rescue/parallel-session-wip`) were pushed, so no work now lives only on this
machine. The standing instruction from the user is that **every patch goes to
GitHub** — commit, then `git push origin main`, every time.

**No release has been published yet.** This matters for the next item: the
update check is correct, verified, and invisible until a release exists at
`https://github.com/blex1nc/incdaw/releases`. Publishing 0.9.0 from
`dist/INCDAW-0.9.0.dmg` is what switches the feature on for everyone.

### The update check (D-038)

INCDAW reads its own public release feed at launch — at most once a day — and
from **INCDAW → Check for Updates…** at any time. It **checks; it does not
update itself**: "Download" opens the release page in a browser. A full
auto-updater was rejected rather than deferred, because it is a signature
scheme and an attack surface, and the problem to solve is that the user does
not KNOW.

- `app::UpdateCheck` is pure — feed parsing, version ordering and the cadence
  decision, with no I/O, so 26 assertions cover the two failures nobody
  notices: reporting "up to date" when it is not, and nagging about a skipped
  version.
- `platform::Http` is the only socket in INCDAW: one HTTPS GET, one timeout,
  one callback on the main thread. Ephemeral session, no cookies, no cache, no
  credential store. Never reachable from the audio thread.
- `AppSettings::updates` carries the launch preference (**on** by default),
  the last-checked stamp and the skipped version. An addition, so the settings
  format stays at version 1.
- **No dependency was added** (CLAUDE.md §41): NSURLSession is Foundation,
  already linked. Nothing entered `third_party/`.

Verified against the live endpoint, not only in tests: launched against
INCDAW's own feed ("no answer: no releases have been published yet", no
dialog, clock unstamped), then with the URL temporarily pointed at a
repository that has releases ("1.13.2 is available", alert presented), then
restored.

**The default is ON and that is a decision, not an oversight.** The user asked
for an automatic check. If they change their mind, it is one checkbox in
Settings, and D-038 records the reasoning in full.

### Two sessions worked in this checkout at once

This increment and the workspace increment above it were written by two Claude
sessions in the same working tree, coordinating over cross-session messages
and committing with explicit paths rather than `git commit -a`. If that
happens again: agree who holds which files BEFORE editing, and never stage
with `-a`, or one session's half-written file lands in the other's commit.


## 2026-08-20 — the work lines merged, the hum killed, the design finished

Three sessions had worked from `main` in parallel worktrees and none had
merged back. They are one line now (this branch), and the two things the
user asked for on top of it are done.

### The consolidation

Merged, in this order, with the conflicts resolved semantically:

1. `claude/onay-devam-6548af` — UI build-out increments 1-10 (project
   lifecycle safety, the generic insert parameter panel, the export
   options dialog, mapping/zone/instrument editors, touch and latch
   automation, insert reordering, the lookahead limiter, the spectrum
   analyzer). Fast-forward.
2. `claude/continue-after-analysis-0b7236` — the FL2026 feature wave
   (chord and note tools, clip splitting, markers and regions, sidechain
   routing, EBU R128, modulation effects and the transient splitter,
   WSOLA time stretching, the slicer, the Browser, Audio Unit hosting).
   19 conflicting files.
3. `claude/fl-garageband-ui-design-99cf4d` — the drawn design language
   (Theme, ControlBarView). 8 conflicting files.

The decisions that were NOT textual:

- **Project format**: both lines had claimed 1.5. Instrument parameters
  keep 1.5; markers, regions and stereo width became **1.6**, with the
  additive migration and `tests/fixtures/v1.6/` rebased to match.
- **D-034** stays with instrument parameter values; the design language
  became **D-035** (Theme.h and ARCHITECTURE.md follow).
- `plugins::HostedPlugin` gained `readParameter` and
  `refreshLatencyIfChanged` as virtuals with honest defaults, so the
  live panels and latency.changed work over the AU-era interface.
- The builtin catalogue kept the sample-rate-taking factory and gained
  the five effects from the other line; `CompressorEffect` kept both its
  gain-reduction meter and its external key.

`claude/kalindan-devam-f7d82f` was NOT merged: its single commit (Phase
U1 project safety) is superseded by UI build-out increment 1.

### The hum at idle — fixed

Opening INCDAW made a sound. The default project's first note is on tick
0, the playhead parks on tick 0, and every block re-triggered it.

Stop does not stop the callback, so the transport keeps handing the
graph one segment while stopped — with the SAME playPosition every
block. `InstrumentNode` re-collected the note under the playhead ~94
times a second (hard-killing the voice in between, because a parked
position looks like a seek to the discontinuity check) and
`AudioClipNode` replayed the frames under the playhead forever.

`ProcessContext::playing` now carries the transport's state to every
node. Timeline readers render nothing while parked; instrument voices,
effect tails and input monitoring go on; the sequence's notes are ended
ONCE, so a keyboard played into a parked project sustains. Offline
rendering is untouched (the flag defaults to true).

Regression tests: `tests/unit/StoppedTransportTests.cpp`, all three
verified to fail against the old behaviour. Contract written down in
docs/AUDIO_ENGINE.md §5.

### The UI design — finished

- The Browser, insert parameter panel and spectrum analyzer draw through
  the theme; **no view in the shell declares a colour of its own**.
- **Metronome**: `MetronomeNode` (which existed since Phase 3 and was in
  no graph) compiles in when the toggle is on, mixed into the master so
  it can never reach a render. Button, Audio menu, checkmark.
- **Tempo**: drag the readout, double-click to type, Shift+Cmd+T to tap.
  `SetTempoCommand` merges, so a drag is one undo entry.
- **Time signature**: read from the project (it has been in the tempo
  map and the file format since the beginning), pickable from the
  readout, undoable, and the position readout counts bars in it.
- **Channel Rack**: a numbered step ruler over the grid.
- **Mixer**: the insert chain is in the strip — four slots, lamp to
  bypass, name to open, empty slot to add.

Engine change under the tempo work: **a compiled project graph owns its
own copy of the tempo map** (`CompiledProjectGraph::tempoMap`). Nodes
point into it while they render, so the caller's map may be rewritten
the moment the graph is replaced. This is what makes editing the tempo
while audio runs safe; before it, the transport's map was shared with
the nodes and a vector reallocation under a binary search was a crash
waiting for someone to drag a tempo.

### State

621 tests pass (Debug), layering passes, the app builds, launches and
was inspected on screen. `third_party/` was copied into this worktree
from the main checkout (it is gitignored).

### Not done, recorded

- The light theme. The `Ink` indirection allows a second table; the
  scheme is deliberately dark-only for now (CLAUDE.md §25 asks for a
  dark professional interface).
- A tempo MAP editor (tempo changes partway through a song). The model
  and the engine support it; only the readout at tick 0 is editable.
  That belongs to the ruler, not to the chrome.
- Insert reordering is still context-menu only; the inline rack does not
  drag-reorder yet.
- `main` has NOT been moved. This branch is the consolidated line; the
  merge into `main` is the user's call.


## 2026-08-17 — UI build-out, part 1: the design language

The shell is now drawn through one visual vocabulary instead of per-pane
greys — FL Studio's density inside GarageBand's calm, drawn from primitives,
nothing copied from either (docs/DECISIONS.md D-034).

Landed:

- `src/ui/macos/Theme.{h,mm}` — palette (`Ink` roles), metrics, type, and the
  drawing vocabulary: panel, well, step pad, region, transport button, knob,
  slider, fader, meter, toggle, LCD, tab, playhead. Every pane draws through
  it; no view declares a colour of its own.
- `src/ui/macos/ControlBarView.{h,mm}` — control bar (rewind/stop/play/record/
  loop, PAT·SONG switch, centre display with bar.beat.tick + tempo + mode +
  material + record lamp, editor tabs, CPU and OUT meters) and the hint bar.
  Replaces the two NSSegmentedControls and the status NSTextField.
- Rounded corners in the Piano Roll's Metal renderer (SDF in the fragment
  shader, still one instance per rect, one draw call); the layer now declares
  sRGB so the GPU pane matches the AppKit panes.
- Pattern clips preview their notes in the Playlist.
- Rotating default colours for new channels/patterns/tracks/mixer nodes
  (`project::Project`), asserted by a test in PatternTests.cpp.
- Dark appearance on the window and on hosted plugin editor windows.

Not done, deliberately (each is a feature, not a repaint — plan before doing):

- Tempo is read-only in the display. Editing it needs an undoable command
  (`SetTempoCommand`) plus a tap-tempo path; the display is ready for it.
- No metronome button: `engine::dsp::MetronomeNode` exists but is not wired
  into the compiled graph, and a button that does nothing is worse than none.
- No time-signature model yet — the display shows 4/4 because the transport
  assumes it.
- Channel Rack has no step ruler above the grid, the Mixer shows no insert
  slots inline (context menu only), and there is still no Browser pane.
- The scheme is dark-only. The `Ink` indirection allows a second table.

Verification at the time of the commit: 484 tests pass, layering check passes,
the app builds and launches, and every pane was inspected on screen. Scripted
clicks on the control bar were NOT used to verify hit-testing — automated
clicks were landing outside the app window — so the transport buttons' click
routing is verified by construction (draw and hit-test share one layout
function), not by an automated run.

Note for this worktree: `third_party/` is git-ignored and absent here; it is
symlinked to the main checkout so the CLAP headers resolve.

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

  Phase 13 remaining (recorded, not blocking Phase 14+):
    - params->flush() while the engine is idle, plugin-originated changes
      (out_events -> recordable automation), clap_host_state.mark_dirty,
      clap_host_latency.changed -> recompile. These are the remaining
      host-callback halves of §5/§6/§8.
    - AU, then VST3 (D-007 order) — DEPENDENCY-GATED: each needs its SDK
      vendored, and the constitution (§41) requires the user's approval
      for any new dependency. Do not start these without it.
    - a generic parameter surface over discovered PluginParameterInfo
      (UI listing every automatable parameter of a slot).
    - sample-accurate event splitting inside the block, once the
      AutomationNode itself evaluates finer than per-block

  PHASE 14 IS STARTED (continuous execution authorized 2026-08-15,
  including "do all phases without stopping"). Part 1 done: the sampler
  core — engine/instrument/Sampler with SamplerZone (shared immutable
  sample, root key, key/velocity ranges, start/end slice, reverse, gain),
  rate-based repitch via linear interpolation, cross-rate resampling,
  SimpleSynth-style ADSR and stealing, 64 voices, layering by overlapping
  zones. setZones is build-time-only by contract (a zone edit is a graph
  rebuild, like inserts). 10 tests in tests/unit/SamplerTests.cpp.

  Phase 14 part 2 is DONE: sustain loops (loopStart/loopEnd within the
  slice, cycling through release; the envelope ends the voice) and the
  crossfaded seam (last N frames blend toward the pre-loopStart material
  the wrap lands on — continuous by construction). Unfit loops are
  ignored, not repaired; reverse zones do not loop yet.

  Phase 14 part 3 is DONE: the sampler reached the model. All four design
  gates resolved and tested (11 new cases, 435 total):
    (a) Channel::samplerZones (ChannelSamplerZone: asset EntityId + the
        mapping numbers) -> project format 1.3, additive; frozen fixtures
        for BOTH v1.2 (the standing gap) and v1.3 now exist.
    (b) plugins::Format::builtin + builtinSampler()/builtinSimpleSynth()
        ("builtin:incdaw.sampler" / "builtin:incdaw.simplesynth"). Empty
        identifier still means "no instrument yet" -> default synth.
    (c) The COMPILER resolves zones (it already had asset resolution for
        clips, now hoisted and shared); builtins are constructed by the
        compiler, and the InstrumentFactory signature is UNCHANGED — it
        remains the seam for hosted formats only (D-028).
    (d) engine::SampleCache, D-032: decoded audio keyed by (path,size,
        mtime), owned by the shell (_sampleCache), passed via
        GraphCompileOptions::sampleCache; sampler zones and clip preloads
        both resolve through it. Chosen over persistent sampler instances
        deliberately — rationale in DECISIONS.md D-032.
    Degradation contract: missing file / unknown asset id -> warning +
    the sampler stays in the graph silent (reconnecting must not change
    topology); unknown builtin uid -> warning + silent channel. Cross-rate
    zones are ALLOWED (sampler repitches by rate); clips still refuse.

  PHASE 14 IS COMPLETE (2026-08-16). Parts 4-6 in one session:
    - LoadSampleCommand + Channel Rack "Load Sample…" (undoable; asset
      shared if the file is already in the project; redo keeps the id).
    - Per-voice state-variable filter (off/LP/HP/BP, cutoff, resonance)
      and one retriggered sine LFO (depth-controlled pitch and cutoff
      destinations). Realtime-safe setters, like the envelope. NOT yet
      per-zone: filter/LFO/envelope are per-instrument. NO UI for filter/
      LFO parameters yet — engine + setters only.
    - Streamed zones: engine/instrument/SamplerStream.{h,cpp}. Head
      (options.samplerHeadFrames, default 65536) decoded resident + pool
      of 4 AudioStreams per zone, claimed wait-free at note-on, windows
      steered to the hand-over point. More held notes than slots degrade
      to head-only voices (silent past the head), never block/allocate.
      Forward unlooped zones stream past the clip threshold; looped and
      reversed zones preload whole regardless of size (a loop must be
      resident). Compiler decides; engine enforces forward-only.
    - EXIT CRITERION MET, measured in SamplerStreamingTests.cpp: velocity
      layers across the keyboard, ~3 s streamed, ZERO underruns (counted
      by the streams), late-window RMS at the exact expected layer mix.
      445 cases / 177,464 assertions green in Debug and Release.
    Remaining sampler niceties (recorded, non-blocking): per-zone
    envelopes/filters/LFOs, more LFO waveforms, streamed-zone loops
    (requires resident loop span), a zone-mapping editor UI, filter/LFO
    parameter UI + automation registration (ParameterRegistry).

  PHASE 15 IS COMPLETE (2026-08-16). The builtin DSP suite (D-033):
    - engine/dsp/effects/: BuiltinEffect base (Node + ParameterSink +
      StateIO; atomic params; versioned id-keyed state blob) and ten
      effects: Utility, Filter (SVF), EQ 3-Band (RBJ), Saturator,
      Compressor, Limiter, Gate, Delay, Reverb (Schroeder), Analyzer.
    - Builtin effects ARE inserts: PluginSlot{builtin, "incdaw.<uid>"}.
      ONE compiler branch builds them from the catalogue
      (BuiltinEffects.cpp); everything downstream — chains, bypass,
      state files, automation, PDC, mixer UI — is the hosted-plugin
      machinery untouched. Mixer context menu: "Add Built-in Effect".
    - ParameterRegistry::registerBuiltinEffects() registers every
      catalogued parameter under the plugin key scheme; the shell calls
      it once at launch.
    - Exit criterion met: bit-exact null tests at transparent settings
      + independent reference implementations per effect
      (BuiltinEffectTests.cpp) + no-special-casing proofs
      (BuiltinInsertTests.cpp). A real limiter ordering defect was
      caught by its reference test and fixed (recover-then-clamp).
    - 461 cases / 286,163 assertions green in Debug and Release.
    Not yet: builtin effect UIs (parameters editable only via state/
    automation today — a generic parameter panel is the natural Phase
    16/UI-phase companion), spectrum analyzer (level analyzer only),
    lookahead limiting (would need latency reporting, which the insert
    machinery already supports when it comes).

  PHASE 16 IS COMPLETE (2026-08-16). MIDI hardware and controller linking:
    - MidiMapping in the MODEL (format 1.4, fixture v1.4): registry key +
      target entity + invertible normalised range. Add/Remove commands;
      re-learn replaces undoably.
    - engine/midi/MidiMapNode compiles from project mappings through the
      SAME resolveApplier as automation lanes (factored out in
      ProjectGraphCompiler — one resolution for lanes and knobs).
    - Instruments are in the parameter system now:
      Instrument::parameterSink, SamplerParam/SimpleSynthParam tables in
      engine/instrument/BuiltinInstruments.{h,cpp},
      registerBuiltinInstruments(); a lane/mapping targeting a CHANNEL
      resolves to its instrument's sink.
    - Learn mode: MidiInput publishes its last CC via a packed atomic
      (lastControlChange); the mixer menu arms learn per node
      (volume/pan); the shell's housekeeping completes it as commands and
      rebuilds. "Forget MIDI Mappings" per node.
    - EXIT CRITERION met in MidiMappingTests.cpp: mapped CCs drive mixer
      volume + sampler cutoff + insert gain through one compiled graph,
      then again after save/load. 466 cases / 286,224 assertions green.
    Deferred, recorded: MIDI clock/sync and hardware feedback need MIDI
    OUTPUT in platform/ (only input exists); channel-strip learn is
    mixer-menu only (channel rack learn later); mapping list UI.

  PHASE 17 IS COMPLETE (2026-08-16). Rendering and export:
    - project/OfflineRender: same compiler, same graph, same block loop
      as the callback — no offline DSP fork to drift. Master / stems
      (mixer-node solo on a model copy) / tracks (channel solo) /
      regions; tail, normalize (peak -> exactly 1.0), deterministic TPDF
      dither, pcm16/24/float32. Offline NEVER streams (determinism).
    - engine/dsp/Resampler: offline windowed-sinc SRC, measured < -60 dB
      error. engine/audio/AiffFile: PCM 16/24 writer.
    - engine/midi/SmfFile + project/MidiFile: SMF-1 write/read; export
      flattens the ARRANGEMENT via the same PatternCompiler playback
      uses (seeded probability included); import creates a pattern +
      channel per track. File menu: Export Audio/Export MIDI/Import MIDI
      (import is deliberately NOT undoable yet — recorded).
    - EXIT CRITERION met in RenderTests.cpp: offline render equals a
      captured realtime-style drive float-for-float, every sample.
    - 478 cases / 1,136,905 assertions green in Debug and Release.
    Deferred, recorded: FLAC/MP3 need a dependency decision (§41);
    AIFF reading; SMF CC/pitch-bend import; export options UI (the menu
    renders master/float32/WAV at the engine rate — stems and formats
    are library-ready but have no dialog).

  PHASE 18 IS COMPLETE (2026-08-16). Performance, measured first:
    - tools/bench (incdaw-bench target): rebuild latency, render
      throughput, per-effect cost, resampler throughput. Reproducible;
      numbers in docs/PERFORMANCE.md §7.
    - Baseline: rebuilds 0.2 ms @ 64 channels, render ~70x realtime,
      effects < 0.1% of block budget each — measured and deliberately
      NOT optimised (§27).
    - ONE optimisation, with before/after: the resampler kernel is now
      a 512-phase table (was per-tap transcendentals): 547.9 -> 18.6 ms
      for 10 s stereo, 29x. Quality regression test unchanged.
    - Bench bug found and fixed: midpoint parameters measured the EQ's
      BYPASS (0 dB mid gain = skipped band); now 75%-of-range. Honest
      EQ cost 15.6 ns/frame.
    Instruments.app profiling still needs full Xcode — recorded, not
    forced (PERFORMANCE.md §6).

  PHASE 19 IS COMPLETE (2026-08-16). QA:
    - FuzzTests.cpp: deterministic (seeded) corruption + truncation of
      project.json / manifest.json / pattern files / WAV / SMF. Contract:
      error or succeed, never crash. No crashes found.
    - StressTests.cpp: 96ch/24-pattern/~12k-note/192-clip project ->
      save/load identity, full compile, finite non-silent render; 400
      edit-rebuild-process cycles stay exact.
    - Crash matrix: scan crashes isolated out-of-process + blacklisted
      (Phase 13). In-process PROCESSING crashes are unsurvivable BY
      DESIGN until sandboxed hosting — open item, recorded, not a
      defect class with a known trigger.
    - EXIT CRITERION met: whole suite green (483 cases / 1,335,729
      assertions, Debug + Release); no known crash-class defects open.
    TESTING.md status updated to reality.

  PHASE 20 IS COMPLETE (2026-08-16). Release engineering:
    - Version 0.9.0 (CMakeLists project VERSION + app/Version.cpp, one
      source of truth stamped into binary/bundle/DMG).
    - docs/RELEASE.md: cutting a release, first-launch on another Mac
      (Gatekeeper vs ad-hoc signature, D-009), update process, 0.9.0
      release notes with the honest known-limits list.
    - dist/INCDAW-0.9.0.dmg built, ad-hoc signed, signature and
      checksum verified. The literal exit criterion (a teammate installs
      on a DIFFERENT Mac) is the one step this machine cannot execute;
      the documented procedure it would follow is complete.

  ALL TWENTY PHASES ARE COMPLETE. What comes next, per the user's
  stated plan (2026-08-16): the UI BUILD-OUT — FL-Studio-class
  workspaces over the finished core. The engine-side surface the UI
  will bind to is ready: CompiledProjectGraph handles (strips, meters,
  instruments, insert state), the command registry, ParameterRegistry
  (every builtin + hosted parameter), MIDI learn plumbing, the offline
  renderer, SMF exchange, and the sample cache. The increment plan is
  in docs/ROADMAP.md ("After the phases — the UI build-out").

  UI BUILD-OUT INCREMENT 1 IS DONE (2026-08-16): project lifecycle
  safety — the "unsaved work is lost silently" era is over.
    - Dirty tracking: the housekeeping undo-depth watch marks the
      document edited on every command-based mutation (undo/redo
      included); MIDI import (the one command-less mutation) marks it
      explicitly. Save/Open/New clear it; the close button shows the
      standard dot (documentEdited).
    - Guards: applicationShouldTerminate + Open + New all run one
      confirmDiscardChanges (Save / Cancel / Don't Save).
    - File > New (⌘N): seedStarterProject (extracted from launch) +
      the same adoption path Open uses
      (adoptLoadedProjectRestoringStateFrom:, which now takes the
      package to restore plugin state from — the autosave when the
      autosave was opened, empty for New).
    - Autosave every 120 s while dirty: saved projects ->
      "<stem>.autosave.incdaw" beside the project; unsaved ->
      Application Support/INCDAW/Autosave/Untitled.autosave.incdaw.
      Never touches dirty/title/recents; failures NSLog, never alert.
      Open offers a strictly-newer sibling autosave (⌘S still targets
      the real path; opening the autosave marks dirty). A leftover
      Untitled autosave is offered at launch (normal quit deletes it;
      only a crash leaves one; successful save deletes it too).
    - Open Recent: last 10 in user defaults (INCDAWRecentProjects),
      Clear Menu, entries that fail to load are pruned.
    - app/ProjectSession.{h,cpp}: headless decisions (updatedRecents,
      autosavePathFor, autosaveIsNewer), 8 cases in
      tests/unit/ProjectSessionTests.cpp. 491 cases green Debug and
      Release (built Debug here; suite green 2/2 ctest).
    - Verified live: launch (starter seeding via the extracted
      method), clean AppleScript quit with NO dialog (guard does not
      false-positive on a clean project).
    KNOWN LIMITATION (recorded): a parameter changed only inside a
    hosted plugin's own editor window does not mark the document
    dirty — invisible to the shell until clap_host_state.mark_dirty
    lands (PLUGIN_HOST §6 host-callback work).

  UI BUILD-OUT INCREMENT 2 IS DONE (2026-08-16): the generic insert
  parameter panel — the ten builtin effects have a UI, and "Open
  Editor" never dead-ends.
    - ui/macos/InsertParameterPanel.{h,mm}: label + slider + value per
      parameter (stepped -> tick marks), scrolls past 420 pt. Holds row
      DATA and a write block only; every write re-resolves the slot
      against _live, because sinks die with their graph.
    - Shell: "Open Editor" opens the CLAP GUI when hasEditor(), else
      the panel (builtins, editor-less or bypassed hosted slots).
      _panelWindows/_panelObservers mirror the editor tables; vanished
      slots close their panels in the rebuild sweep; writes markDirty.
    - CompiledProjectGraph::insertSinkFor(slot) — the compiler's own
      insertSinks map (lanes/knobs resolution) exported per slot.
    - BuiltinEffect::decodeState — loadState's decoder factored out;
      the panel reads current builtin values via StateIO::saveState +
      this decoder (one decoder, no drift).
    - ClapInstance::readParameter — CLAP params.get_value, main-thread,
      params_ extension pointer now retained on the instance; hosted
      panels show live values, defaults when a plugin cannot answer.
    - FIXED (was latent since Phase 15/16): builtin insert nodes are
      recreated at defaults by EVERY compile, so values set on the live
      node (MIDI-mapped knobs; now the panel) reset on any note edit.
      project::captureBuiltinInsertState/restoreBuiltinInsertState
      (PluginStateFiles) carry the blobs in memory across the swap in
      rebuildGraph. Hosted instances persist via D-031 and are NOT
      re-fed their blobs. adoptLoadedProjectRestoringStateFrom clears
      _live and closes all panels FIRST — entity ids restart per
      project, and a stale handle table could alias a new slot id onto
      an old node (values would leak across projects).
    - Tests: 4 new cases — sink exposure + write lands (decoded through
      the shared decoder), rebuild carry-over (fresh node proves the
      reset, restore proves the fix, vanished-slot restore is a no-op),
      hosted readParameter (default, after a processed change, unknown
      id refused). 495 cases green Debug + Release. Launch + clean-quit
      smoke verified live.
    DEFERRED to increment 4 (recorded in ROADMAP): builtin INSTRUMENT
    parameters — instruments have sinks but no StateIO, so a panel
    value would silently vanish at save/load; persistence first.
    NOT undoable (recorded): panel writes, same as a plugin's own GUI.
    OBSERVED, NOT FIXED (pre-existing, out of scope §40): an open CLAP
    editor window survives a slot whose uid changes in place (the
    rebuild sweep only closes windows whose slot KEY vanished, but a
    changed uid recreates the instance under the same key, D-031) —
    the editor window can outlive the instance it was opened on.
    Recheck when editor lifecycle is next touched.

  UI BUILD-OUT INCREMENT 3 IS DONE (2026-08-16): the export options
  dialog. File > Export Audio… fronts every RenderOptions field:
  master / stems (per non-master mixer node, stemMixerNode) / tracks
  (per channel, soloChannel), WAV/AIFF, pcm16/24/float32, target rate
  44.1-96k (resampler), tail seconds, normalize, dither, loop-range
  region (transport loopStart/loopEnd). Multi-file targets go into a
  chosen directory via app::session::exportFileName (sanitise +
  case-insensitive dedupe, tested — 496 cases green Debug + Release).
  The render loop is synchronous main-thread, no progress/cancel —
  recorded polish. The dialog itself is interactive and rests on the
  library's RenderTests plus the launch/quit smoke; a scripted
  UI-drive of the dialog was not attempted (prior sessions recorded
  synthetic clicks landing in whatever app is frontmost).

  UI BUILD-OUT INCREMENT 4 IS DONE (2026-08-16): mapping, zone and
  instrument editors — the increment-2 deferral resolved by D-034.
    - PERSISTENCE FIRST (D-034, format 1.4 -> 1.5, additive, frozen
      fixture v1.5): Channel::instrumentParameters ({id, plain value});
      the COMPILER applies them through instrument->parameterSink()
      right after construction. Model = source of truth at every
      rebuild (the mixer's rule): survives rebuilds and save/load, and
      makes the edits undoable. Hosted instruments will use their
      instrumentStateFile blob instead when they arrive — model values
      are builtin-only.
    - SetInstrumentParameterCommand (ChannelCommands; mergeable;
      undoing a never-touched parameter REMOVES the entry — "at the
      default" and "stored at the default" stay distinct).
    - Rack context menu "Edit Instrument…" -> the increment-2 panel
      reused verbatim (rows from engine::findBuiltinInstrument — new
      catalogue lookup; empty uid resolves to the reference synth);
      writes = executeMerging + rebuildGraph. Panels are keyed by
      channel id in the same _panelWindows table (entity ids share one
      space, no collision); the rebuild sweep's valid-key set now
      includes channels.
    - Rack context menu "Sampler Zones…" -> zone editor window (one at
      a time, _zoneWindow/_zoneChannelKey/_zoneRows): per-zone rows
      (asset name, root, key lo/hi, vel lo/hi, gain, reverse, Remove) +
      "Add Sample Layer…". Whole-row commit -> SetSamplerZoneCommand
      (mergeable) / RemoveSamplerZoneCommand; layering ->
      AddSamplerZoneCommand (refuses non-sampler channels; LoadSample
      remains the convert-gesture; both share ONE extracted
      asset-minting helper in SamplerCommands.cpp). Content rebuilt
      from the model after every change, clamps visible. Zone
      start/end/loop fields deliberately not in the first row layout.
    - Audio > MIDI Mappings… -> _mappingWindow: "CC n (ch) ->
      parameterKey · target display name" per row + undoable Remove;
      refreshed on every rebuild while open (learn/forget elsewhere
      stay in sync). Rows address mappings BY ID captured at refresh,
      never by live index.
    - adopt closes zone + mapping windows (cross-project ids);
      rebuild sweep closes the zone editor when its channel vanishes.
    - Tests: 7 new cases in InstrumentParameterTests.cpp — compile
      application proven EQUAL to a live sink write (and audibly
      different from defaults), format round-trip, frozen v1.5 fixture,
      v1.4 now asserts migrated/migratedFrom (bump maintenance),
      command undo/merge semantics, zone edit/remove undo, add-layer
      asset sharing + undo-removes-minted-asset, catalogue lookup.
      502 cases green Debug + Release; launch/quit smoke clean.
    KNOWN LIMITS (recorded): zone editor is numeric (no graphical
    key-range view; start/end/loop uneditable in UI); instrument panel
    covers builtins only; panel value rows do not live-refresh while
    automation moves the same parameter.

  UI BUILD-OUT INCREMENTS 5-10 ARE DONE (2026-08-16, one session;
  commits 77e6b90, 9670a15, bce6766, 40a4a8f, 3e9e902, 4654f1e):
    5. MoveInsertCommand + mixer Move Up/Down; live panel refresh at
       5 Hz (builtin decode / hosted readParameter / instrument model,
       never during a drag); clap_host_latency.changed implemented
       end to end (host ext -> atomic flag ->
       PluginInstanceManager::refreshChangedLatencies in housekeeping
       -> recompile; latency test plugin's state load rings it).
       flush-while-idle recorded NOT APPLICABLE in PLUGIN_HOST §5 —
       no code path can queue into an instance whose process is not
       running (bypassed slots have no sink in the graph).
    6. RenderOptions.progress (every 32 blocks; false = "cancelled");
       shell exports on a background thread over COPIES of project +
       tempo map behind a modal progress window polling shared atomics
       (modal runloop does not reliably drain the main GCD queue).
    7. Touch/latch (the 11b deferral): session-level segment closing.
       Write = one segment first-to-last move; touch = segment per
       drag (MixerView mouseUp -> onParameterGestureEnded); latch =
       hold last value to finish(endTick). Menu: Record Automation >
       Off/Write/Touch/Latch; mode switch lands the pass first.
    8. Editor clipboard: edits::extractRegion/deleteRegion/insertAudio
       (rate/channel mismatch refused); DeleteAudioRegionCommand +
       InsertAudioCommand (piece carried for redo); copy is not a
       command; _audioClipboard app-local. Audio menu Cut/Copy/Paste/
       Delete Selection.
    9. LookaheadLimiterEffect "incdaw.limiterla": NEW catalogued
       effect, classic limiter untouched (Phase 15 node-null intact).
       Fixed 2 ms window BY DESIGN (latency read at topology, before
       prepare; a knob-driven latency would stale PDC between
       rebuilds) -> makeBuiltinEffect now takes sampleRate. Sliding
       max via preallocated monotonic deque. Proven: transparent =
       pure delay of exactly the window; step never exceeds ceiling;
       insert reports 96 frames @48k into graph latency.
   10. Spectrum: engine/dsp/Fft (radix-2, tables in setSize, transform
       allocation-free, proven vs naive DFT); AnalyzerEffect
       accumulates 2048-sample Hann mono downmix, publishes dBFS bins
       via seqlock double buffer, readSpectrum on the UI thread;
       CompiledProjectGraph::insertNodeFor (built insert NODES by
       slot); ui/macos/SpectrumView (log-f path, decade gridlines),
       opened by "Open Editor" on an analyzer slot, repainted at the
       full housekeeping rate.
    514 cases green Debug + Release; launch/quit smoke clean.

  REMAINING — BY GATE (ROADMAP "Remaining, by gate" is authoritative):
    - §41 DEPENDENCY APPROVAL REQUIRED: AU then VST3 hosting; FLAC/MP3
      export. DO NOT start without the user's explicit approval.
    - PLATFORM+HARDWARE: MIDI output/clock/sync/feedback (platform/
      has input only; correctness claims need hardware).
    - OWN PROGRAM: sandboxed plugin processing (crash matrix's open
      item since Phase 19).
    - UNGATED FUTURE INCREMENTS: graphical zone view, per-zone
      envelopes/filters/LFOs, out_events -> recordable automation,
      editor markers/regions, sample-accurate intra-block automation.

  SINCE THEN (branch claude/fl-studio-2026-features, continued on
  claude/continue-after-analysis): the UI build-out ran in ten
  increments — project lifecycle safety, the generic insert parameter
  panel, the export options dialog, mapping/zone/instrument editors,
  insert reordering with live panels and latency.changed, export
  progress and cancel, touch and latch automation modes, editor
  cut/copy/paste, the lookahead limiter, the spectrum analyzer. The FL
  Studio 2026 gap analysis (docs/FL2026_GAP.md — read it first, it holds
  the status table) then drove eight feature blocks, P1-P8: the chord
  toolkit, note tools, clip split with markers and regions (format
  v1.5), functional sidechain routing, LUFS with stereo separation and
  true pre-fader sends, chorus/flanger/phaser with the transient
  splitter, the WSOLA time-stretch subsystem, and the slicer.

  P9 (THE BROWSER) AND P10 (AUDIO UNITS) ARE COMPLETE (2026-08-17).
  docs/FL2026_GAP.md holds the closed table; the notes below are what the
  next session should know about how they were built.

  P9 — THE BROWSER — in four parts:

    - Part 1, app::Browser: classification (a project package and a
      .clap bundle are items, never folders to walk into), folders-first
      listings with dot-files hidden, recursive search capped at 500
      results and 8 levels, favourites, recents, JSON persistence
      staged-and-renamed. canDecodeAudio is deliberately separate from
      the kind — a .flac IS audio and the row says so, but WAV is all
      engine/audio reads today, so the drop must refuse with a reason.
      8 test cases in tests/unit/BrowserTests.cpp.

    - Part 2, INCDAWBrowserView: a stock NSOutlineView over that model
      (a file tree is what AppKit's outline view is for; the custom
      drawing in this shell is for musical surfaces). Leftmost in the
      workspace, View > Browser (Cmd+B), per-keystroke search, context
      menu for favourite / reveal / add root / remove root / refresh.
      The pane opens nothing itself: a double-click hands the path to
      the shell, which is why openProjectAtPath: and importMidiFromPath:
      were factored out of the panel handlers. Roots, favourites and
      recents live in Application Support/INCDAW/browser.json beside
      plugins.tsv — installation state, never project state.

    - Part 3, engine::AuditionPlayer: the preview is mixed by the ENGINE
      after the project graph, not compiled into it — it sounds with the
      transport stopped and costs no rebuild. The audio thread reads a
      raw pointer; the shared_ptr is released by collect() only after
      the block counter passes the swap (the retired-graph grace). play()
      silences first and re-arms last, so a file swapped mid-flight can
      never be read at the previous playhead. NaN containment moved out
      of the graph branch to cover it.

    - Part 4, the drops: LoadSampleCommand onto a channel,
      ImportSampleAsChannelCommand onto empty rack space,
      ImportAudioClipCommand onto a playlist lane (tick -> frame
      conversion lives in the command, D-013). Asset import is one
      shared helper now, app::AudioAssetImport — probe before mutating,
      share a file already in the project, keep the created asset's id
      and index so redo cannot orphan what was written above it.

  P10 — AUDIO UNITS — and the interface it forced:

    - plugins::HostedPlugin is now what PluginNode, the instance
      manager, PDC, plugin state files and the editor windows are
      written against. ClapInstance and AudioUnitInstance implement it.
      A third format implements it and nothing else.
    - platform::AudioUnitHost holds everything CoreAudio (the layering
      checker is right: plugins/ must not know its OS). Enumeration runs
      no plugin code, so AUs need NO scan and appear in Add Insert
      immediately; instantiation is in-process, like CLAP hosting.
    - Deliberately not in it, and the natural next steps: AU instruments
      (the insert path is stereo effects), custom Cocoa views
      (kAudioUnitProperty_CocoaUI — the generic view is what ships
      today), out-of-process AU instantiation, and an AU-specific test
      of automation and of state through a project save (both travel the
      shared seams, which are tested for CLAP).

UI build-out increment 11 — the workspace (2026-08-22):

  What it added, and what each thing was blocking:

    - app/AppSettings + ui/macos/SettingsWindow, reached with Cmd+, —
      output and input device, sample rate, block size, MIDI sources,
      "open the input at launch". Applying reopens the device and the
      MIDI client with the playhead preserved in TICKS (a sample-rate
      change redefines what a frame position means). The status line is
      read back from the OPEN device: a refused rate or a rounded block
      size must never be reported as the value that was asked for. A
      device that will not open falls back to the system default rather
      than leaving the application silent. Settings are their OWN
      versioned file in the application support directory, never project
      data — D-036 says why, and the short version is that a project
      carrying its author's interface id is unopenable on a second Mac.

    - platform::MidiDevice is finally OPENED by the shell. This was the
      one missing link in a chain complete and tested since Phase 5:
      engine::MidiInput existed, the lock-free queue existed, MIDI learn
      polled it — and nothing ever fed it, because nobody called
      MidiDevice::create() outside the tests. A keyboard reached CoreMIDI
      and stopped there. The client is closed in
      applicationWillTerminate BEFORE the engine goes away: it delivers
      on its own thread and holds a reference to the engine's input.

    - ui/macos/CommandPalette, reached with Cmd+K. It owns no catalogue:
      the shell walks the menu bar and hands it entries each time it
      opens, so a command cannot be listed there and missing from the
      menu, or carry a different shortcut in the two places.

    - app::registerStandardActions, in app/ rather than in the shell so a
      test can reach it. The registry's action table was EMPTY in the
      running application until this: every edit arrived as a command,
      but none of them had an id anything could look up (CLAUDE.md §26).
      Three actions today — add channel, pattern, track. Widening that
      table is how the palette grows; an action belongs in it once it is
      meaningful without a selection.

    - The workspace is restored: window frame, active pane, song mode.
      The frame is only restored if it still intersects an attached
      screen, and the layout is now measured from the window's real
      content size instead of the 1280x800 default constant — otherwise
      a restored smaller window has panes hanging off its edge.

    - Undo and redo route through the shell (D-037) and name what they
      will do. The five panes' own Cmd+Z handlers survive only as the
      fallback for when the menu item is disabled, which happens exactly
      when a text field has focus and the field editor's undo must win.

  Deliberately not in it, and the natural next steps:

    - The panes' Cmd+Z copies are now unreachable in normal use.
      Collapsing them is real work, not a drive-by edit: each also
      refreshes its own view there.
    - The settings window does not offer a channel map, an input-channel
      count, or a per-project sample-rate override. The last one is a
      feature, not an omission — see D-036.
    - Nothing re-enumerates devices on hot-plug. The window rescans when
      it opens and on its Rescan button; a device unplugged while the
      window is open is stale until then.
    - MIDI OUTPUT is still unopened. MidiDevice::sendMessage exists and
      the settings window lists inputs only.
    - No test drives the shell. AppSettings and the action table are
      covered in app/; the wiring in main.mm is covered by building,
      launching, and the settings file round-tripping through a real
      quit — which is what was actually done, not a claim of more.

UI build-out increment 12 — the velocity lane (2026-08-22):

  app::PianoRollModel grew the lane's arithmetic; ui/macos/PianoRollView
  draws and drives it. Nothing else changed.

    - Viewport::height now means the NOTE GRID's height, not the view's,
      with velocityLaneHeight beside it. Every existing caller is
      unaffected because the grid's own geometry is unchanged — but do
      not "fix" height back into the view's height, or the lane will
      draw over the lowest key rows.
    - The lane culls on the note's START tick, deliberately stricter than
      note culling, which keeps a long note that began off-screen left.
      That note's bar would sit where it can be seen and not grabbed; a
      bar is either fully addressable or absent.
    - barAtPoint targets the whole column, not the filled part of the
      stem. A velocity-5 bar is three points tall, and requiring a hit on
      those three points would make the quietest notes the hardest to
      raise.
    - The drag snapshots the selection into _velocityTargets.
      SetVelocityCommand merges only across identical index lists, so a
      selection that changed mid-drag would silently produce one undo
      entry per mouse move.
    - Velocity 0 is unreachable by construction (note-off). The command
      already clamped; the lane clamps too, so the two cannot disagree.

  Deliberately not in it:

    - No CC or pitch-bend lanes. The lane is hardcoded to velocity; a
      lane selector is the next step and wants a model change, not a
      view one.
    - No menu entry for the toggle — E only. The shell's View menu was
      owned by another session's work at the time; adding it is a
      one-line follow-up in main.mm -buildMenu.
    - INCDAWPianoRollView.statusText is still written and read by nobody.
      The lane reports through it like every other gesture; the shell
      builds its own status line and ignores it.

UI build-out increment 13 — the Channel Rack (2026-08-22):

    - The row is ONE line again (rowHeight 32) and reads left to right:
      mute lamp, solo, channel button, volume knob, pan knob, steps. An
      earlier pass in this same session made it two lines and mixer-like;
      that halved how many channels fit on screen, and a rack is read by
      scanning down it. Do not restack it.
    - muteRect and swatchRect are THE SAME RECT. The lamp carries both
      identity (the channel's colour) and state (lit or dark). If you add
      a swatch back, delete the lamp's colour first or the row says the
      same thing twice.
    - stepOffset is the only place the group gaps are computed. The grid,
      the ruler and the hit test all call it. The offset is NON-LINEAR in
      the step index, so any new caller that divides by (stepWidth +
      stepGap) will be wrong by a whole cell near the end of a long bar —
      the hit test estimates with that divide and then corrects, which is
      what the 32-step assertion in ChannelRackTests guards.
    - Gaps are counted from the pattern's step zero, not from firstStep_,
      so scrolling does not move where a bar appears to start.
    - Both knobs drag vertically and RELATIVELY, from the value the drag
      began at (ChannelRackModel::knobForDrag). Reading the current value
      on each move compounds rounding and the knob creeps away from the
      cursor. knobDragTravel is the END-TO-END sweep, so half of it from
      centre reaches an end of a bipolar range.
    - SetChannelPanCommand closes the third instance of a recurring
      pattern here: a property the model, the file format and the
      compiler all support, with no command and therefore no way to set
      it. Channel::pan was compiled at ProjectGraphCompiler.cpp:569 the
      whole time. When adding a control, check the compiler first — the
      audio path is usually already there.
    - Merging cannot tell one gesture from the next: the entry a drag
      opens is the whole drag, and one undo returns to before its first
      point. The pan tests say so explicitly, because the first version
      of them assumed otherwise.

UI build-out increment 14 — the Piano Roll's ruler, ghosts and key
(2026-08-23):

    - Viewport::height is the GRID alone. The view's height is
      rulerHeight + height + velocityLaneHeight, and applyViewportGeometry
      is the only place that divides it. keyToY adds gridTop(); anything
      that computes a row's y without it will draw under the band.
    - Both hit tests refuse points in the ruler (isInRuler). Do not rely
      on yToKey returning something out of range there — it does, but
      that is arithmetic, not a guard, and it stops being true the moment
      the band is taller than a row.
    - Ghosts are the OTHER channels of the same pattern, appended into one
      list via collectVisibleNotes(..., append=true). Their `index` is
      the index in their own channel's event vector, which is meaningless
      here — ghosts are drawn and never hit-tested, and must stay that
      way or a click would move a note in a channel the editor is not
      editing.
    - Text is a pool of CATextLayers over the Metal layer, reset per frame
      by beginLabels/endLabels. The renderer draws rectangles and only
      rectangles; that is what makes ten thousand notes one draw call, and
      adding a glyph path to it would be a change to the thing D-006's
      performance model rests on. Labels are capped at bar numbers and one
      name per octave for the same reason.
    - Scale highlighting reads _keyRootPc and _scale — the SAME state the
      nudge tool uses. If a key-signature control is ever added, it sets
      those two and both follow.

UI build-out increment 15 — the rack's step levels (2026-08-23):

    - A step IS a note, so a step's level is an ordinary velocity edit:
      the rack commits the Piano Roll's SetVelocityCommand. There is
      deliberately no step-level command. If one is ever added, the two
      editors can drift apart on the same MidiEvent, which is the thing
      StepCommands.h exists to prevent.
    - Shift+drag over a lit pad is the gesture. Plain drag still paints,
      and painting must keep working over an empty grid — the level drag
      therefore only begins on a step that is already programmed.
    - The note's index is captured at mouseDown and reused for the whole
      drag. That is safe for exactly one reason: velocity edits do not
      re-sort the event vector. A gesture that moves or adds notes may
      not do this (NoteCommands.h says which commands re-sort).
    - The floor is velocity 1. Zero would be a step that is lit and
      silent; clearing is the right button's job.
    - Right-click over the grid clears a step and does NOT open the
      channel menu. The menu is still there over the row's header. If a
      step context menu is ever wanted, that is the zone to branch on —
      the branch already exists in rightMouseDown.
    - theme::drawStepPad's `level` is defaulted, so any pane that draws a
      pad without one is unchanged.

    Scope expansion, found and NOT taken (CLAUDE.md §40): an on-screen
    keyboard or a click-to-audition step cannot be built yet. Live MIDI
    is merged into EVERY InstrumentNode (InstrumentNode.cpp, the
    context.liveMidi loop) — there is no live-target channel — so one
    preview note would sound every channel in the project at once. That
    is also true of a hardware keyboard today. A live-input target
    (armed channel, or FL's "typing keyboard to piano roll" routing) is
    the feature that has to land first; MidiInput::injectForTesting is
    already the intended entry point for the UI side.

UI build-out increment 16 — the Piano Roll's control strip (2026-08-23):

    - The strip is a SIBLING view (main.mm places it in editorContainer
      and hides it with the pane), not a band inside PianoRollView.
      Putting it inside would spend the Metal pane's text budget on
      chrome; that budget is what D-006's one-draw-call model rests on.
      applyViewportGeometry is untouched and still divides the pane's own
      height between ruler, grid and velocity lane.
    - The editor owns the state; the strip only shows it. Every pick goes
      PianoRollHeaderView -> main.mm -> INCDAWPianoRollView, and the
      strip is then re-read from the editor (syncPianoRollHeader). Do not
      give the strip its own copy — E and G change the same settings from
      the keyboard, which is what onEditorStateChanged exists for.
    - Snap zero means "do not snap" in PianoRollModel already, so the
      picker hands ticks straight over. snapForTicks maps any value to
      the nearest division AT OR BELOW it, which is why a value set from
      elsewhere can never make the strip claim a finer grid than the one
      notes land on.
    - Key and scale are the SAME _keyRootPc and _scale the nudge tool and
      the scale highlighting read (increment 14's note). There is still
      only one key signature in the editor, and now it has a control.

Things to be careful about:

  - third_party/ is gitignored: a fresh clone or worktree has neither
    doctest nor the CLAP headers, and the build fails on the test target
    and on plugins/ until both are fetched (HANDOFF section 5 has the
    doctest command; D-027 has CLAP's).
  - RESOLVED (2026-08-15): the File menu exists — Open/Save/Save As call
    ProjectFile and PluginStateFiles in the documented order (capture before
    save; restore after the rebuild).
    RESOLVED (2026-08-16, UI build-out increment 1): dirty prompt on quit,
    autosave with crash recovery, Open Recent, File > New. Unsaved work is
    no longer lost silently.
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
