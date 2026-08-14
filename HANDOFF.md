# INCDAW — HANDOFF

Version: 0.2
Status: PHASE 0 COMPLETE / AWAITING PHASE 1 APPROVAL
Last updated: 2026-08-14
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

PHASE 0 — COMPLETE (2026-08-14)
PHASE 1 — NOT STARTED

Phase 0 delivered discovery, architecture decisions, and the documentation set.
No source code exists. See docs/ROADMAP.md for phase gates.

Implementation status:

NOT STARTED

Production audio engine:

NOT STARTED — designed in docs/AUDIO_ENGINE.md, target Phase 2

MIDI engine:

NOT STARTED — target Phase 5

Project format:

NOT STARTED — v1.0 designed in docs/PROJECT_FORMAT.md, target Phase 4

UI:

NOT STARTED — target Phase 6

Plugin host:

NOT STARTED — designed in docs/PLUGIN_HOST.md, target Phase 13

Testing:

NOT STARTED — strategy in docs/TESTING.md, infrastructure lands in Phase 1

Release system:

NOT STARTED — DMG pipeline designed (D-009), scripted in Phase 1

Version control:

INITIALISED — git, branch `main` (D-010)

Environment (verified 2026-08-14):

macOS 26.2 · Apple M5 arm64 · 10 cores · 16 GB RAM
Apple clang 21.0.0 · macOS SDK 26.5 · Command Line Tools only (no Xcode.app)
CoreAudio / CoreMIDI / AudioUnit / os_workgroup / Metal / Accelerate all present
CMake and Ninja NOT INSTALLED — blocking for Phase 1
No code-signing identity — DMG will be ad-hoc signed, not notarized (D-009)

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

Phase 0 is complete and approved.

The project has NOT been implemented. There is no source code and no build
system. What exists is the architecture, the decision log, and the phase plan.

Read first:

1. docs/DECISIONS.md    — what was decided and why
2. docs/ARCHITECTURE.md — layers, threading, data model, commands
3. docs/ROADMAP.md      — phases and their exit criteria

Next step: Phase 1 (foundation and build system).

Phase 1 is BLOCKED pending approval to install CMake and Ninja via Homebrew.
Nothing else blocks it.

Phase 1 delivers: CMakeLists.txt, the src/ layer skeleton, the doctest runner,
the layering test, and tools/make-dmg.sh. It delivers NO audio code — the audio
engine is Phase 2.

The approval rule still applies to every phase. Plan, show the plan, stop, wait.

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
