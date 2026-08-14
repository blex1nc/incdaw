# INCDAW — Claude Code Project Constitution

## PROJECT IDENTITY

Project name: INCDAW
Project type: Professional Digital Audio Workstation (DAW)
Primary goal: Build a complete, professional music production application inspired by the workflow and feature depth of modern DAWs, with FL Studio 2026 used as a functional reference.

INCDAW must NOT be treated as a toy DAW, demo, prototype, mockup, UI-only application, or simplified music editor.

The long-term objective is a serious production-grade DAW capable of:

- composing
- MIDI sequencing
- audio recording
- audio editing
- beat production
- sound design
- mixing
- routing
- automation
- mastering
- plugin hosting
- project management
- MIDI hardware integration
- professional export/rendering
- extensibility

The product must be architected so that features can be added without rewriting the audio engine.

---

# ABSOLUTE USER CONTROL RULE

THIS IS THE MOST IMPORTANT RULE IN THIS FILE.

Claude MUST NOT autonomously implement changes.

Claude MUST NOT:

- modify source code
- create implementation files
- delete files
- rename architecture components
- install dependencies
- change dependencies
- change build configuration
- change compiler configuration
- change audio architecture
- change project architecture
- change UI architecture
- run destructive commands
- migrate databases/project formats
- commit code
- push code
- make irreversible changes

unless the user explicitly approves the proposed plan.

The following do NOT count as approval:

- "continue"
- "go ahead"
- "do it"
- "fix it"

unless the user has first been shown the exact implementation plan for the requested operation.

When a task is requested:

1. Analyze.
2. Research.
3. Use Graphify.
4. Inspect the existing architecture.
5. Produce a plan.
6. Clearly state what will change.
7. Clearly state files/components affected.
8. Clearly state risks.
9. Clearly state verification/testing.
10. STOP.
11. Wait for explicit user approval.

Only after explicit approval may implementation begin.

If the user asks a question, answer the question without modifying the repository.

---

# GRAPHIFY IS MANDATORY

Graphify must be treated as a persistent architectural intelligence layer.

Before major:

- architecture decisions
- feature implementation
- refactoring
- debugging
- dependency changes
- subsystem design
- audio engine work
- UI architecture work
- plugin architecture work
- project format work
- performance work

Claude MUST consult Graphify first.

Preferred workflow:

1. `/graphify .`
2. Read `graphify-out/GRAPH_REPORT.md`
3. Query the graph for relevant architecture relationships.
4. Only then inspect raw files required for the task.

Do not blindly grep/read the entire repository when Graphify can establish the relevant structure first.

After meaningful source changes are approved and implemented:

1. update/rebuild Graphify
2. verify architectural relationships
3. check for orphaned or disconnected components
4. record important architectural discoveries

If Graphify is installed, use the installed Graphify skill.

If Graphify is unavailable, DO NOT pretend that it was used.

Instead report:

"Graphify is unavailable in the current environment. I have not claimed graph-based analysis."

Then ask whether the user wants to install/configure it.

Graphify should be treated as continuously useful, not a one-time setup tool.

---

# DEVELOPMENT PHILOSOPHY

INCDAW is a long-term engineering project.

Do not optimize for:

- fastest demo
- shortest code
- fake UI
- placeholder audio processing
- hardcoded behavior
- temporary architecture that cannot scale
- visually impressive but non-functional components

Optimize for:

- correctness
- deterministic audio behavior
- low latency
- realtime safety
- extensibility
- testability
- maintainability
- modularity
- performance
- professional workflow
- backwards-compatible project evolution

Never implement a feature merely because it looks correct.

A DAW feature must actually work in the audio/MIDI engine.

---

# FL STUDIO FEATURE PARITY OBJECTIVE

FL Studio is a FUNCTIONAL REFERENCE.

The objective is to reproduce the capabilities and workflows, NOT proprietary implementation, source code, branding, copyrighted assets, or proprietary plugins.

Do not copy:

- Image-Line source code
- proprietary algorithms
- proprietary assets
- proprietary presets
- proprietary plugin binaries
- copyrighted UI assets
- trademarks/logos
- internal implementation details

INCDAW must use independent implementations and original UI/UX where appropriate.

---

# FUNCTIONAL REFERENCE AREAS

The following feature families must be investigated and eventually represented in the INCDAW architecture.

## 1. CORE TRANSPORT

- play
- pause
- stop
- record
- loop
- pattern/song modes
- metronome
- count-in
- tempo
- time signature
- position
- playhead
- pre-roll
- post-roll
- punch in
- punch out
- start/stop markers
- timeline navigation
- snap
- quantization
- CPU/audio indicators

---

# 2. PROJECT SYSTEM

INCDAW must support:

- new project
- save
- save as
- open
- recent projects
- project recovery
- autosave
- backup
- project metadata
- project versioning
- project migration
- project templates
- project packaging
- sample dependency management
- missing-file detection
- relinking
- project archive

The project format must be versioned from day one.

Never create an unversioned project format.

Example:

INCDAW Project Format:

INCDAW_PROJECT_VERSION = X.Y

The project format must support future migrations.

---

# 3. AUDIO ENGINE

This is a critical subsystem.

The audio engine must eventually support:

- realtime audio processing
- multichannel audio
- stereo
- mono
- surround-ready architecture
- sample-accurate scheduling
- realtime-safe processing
- deterministic transport
- low-latency playback
- audio device abstraction
- input/output device routing
- sample rate
- buffer size
- latency compensation
- plugin latency compensation
- PDC
- offline rendering
- realtime rendering
- bounce
- freeze
- track rendering
- resampling
- interpolation
- oversampling
- clipping detection
- metering

The realtime audio thread must NEVER:

- allocate memory unnecessarily
- perform blocking I/O
- perform network operations
- wait for UI
- use locks that can cause priority inversion
- perform expensive unpredictable operations

Audio thread safety must be treated as a first-class architectural requirement.

---

# 4. MIDI ENGINE

Support:

- MIDI input
- MIDI output
- MIDI recording
- MIDI playback
- MIDI editing
- velocity
- note length
- note start
- note end
- pitch
- modulation
- pitch bend
- aftertouch
- polyphonic aftertouch where supported
- CC
- program changes
- MIDI clock
- MIDI sync
- MIDI routing
- MIDI thru
- MIDI quantization
- MIDI humanization
- MIDI mapping

Architecture should be prepared for future MPE support.

---

# 5. PIANO ROLL

Piano Roll is a first-class editing environment.

Must eventually support:

- note creation
- deletion
- movement
- resizing
- duplication
- multi-selection
- box selection
- lasso
- velocity editing
- note color/channel assignment
- quantization
- scale snapping
- ghost notes
- ghost patterns
- chord tools
- chord generation
- strum
- arpeggiation
- legato
- articulation
- slide notes
- portamento behavior
- note probability
- note repeat
- note grouping
- MIDI CC lanes
- pitch bend lanes
- modulation lanes
- automation/event editing
- snap
- grid customization
- zoom
- horizontal scrolling
- vertical scrolling
- undo/redo

Piano Roll performance must remain usable with very large note counts.

---

# 6. CHANNEL / INSTRUMENT SYSTEM

Architecture must support a Channel-like system.

Each channel can represent:

- software instrument
- sampler
- audio source
- MIDI source
- generator
- plugin
- external MIDI device

Channels should support:

- mute
- solo
- volume
- pan
- routing
- instrument assignment
- MIDI input
- MIDI output
- channel state
- preset
- automation
- color
- naming
- grouping

---

# 7. STEP SEQUENCER

Support:

- step programming
- pattern programming
- velocity
- note repeat
- probability
- per-step parameters
- swing
- accents
- pattern length
- multiple time divisions
- polymetric possibilities
- MIDI generation

---

# 8. PATTERN SYSTEM

The architecture should support pattern-based workflows.

Patterns may contain:

- MIDI
- automation
- controller events
- generated sequences
- other event data

Patterns must be reusable.

Patterns must be independently editable.

Patterns must be placeable into the arrangement.

---

# 9. PLAYLIST / ARRANGEMENT

INCDAW Playlist must eventually support:

- audio clips
- MIDI clips
- pattern clips
- automation clips
- instrument tracks
- audio tracks
- markers
- regions
- clip splitting
- clip resizing
- clip duplication
- clip stretching
- clip reversing
- clip fading
- crossfades
- clip gain
- clip pitch
- clip time stretching
- clip consolidation
- clip grouping
- clip locking
- clip coloring
- track folders
- track groups
- lane management
- overlapping clips
- free clip placement
- track-based workflow
- song arrangement

The architecture must not assume a simplistic 1:1 track-to-mixer relationship.

The routing model must be flexible.

---

# 10. AUTOMATION

Support automation for essentially any automatable parameter.

Automation must support:

- points
- curves
- ramps
- step transitions
- tension
- smoothing
- clip-based automation
- parameter automation
- MIDI automation
- recording automation
- automation recording modes
- automation editing
- automation linking
- parameter mapping
- automation scaling
- automation copy/paste

Automation must be a generic subsystem.

Do NOT build automation separately for every plugin/control.

---

# 11. MIXER

The mixer must support:

- unlimited or architecture-scalable tracks
- inserts
- sends
- returns
- buses
- subgroups
- master
- routing matrix
- sidechain routing
- pre/post routing
- volume
- pan
- mute
- solo
- polarity
- stereo separation
- gain
- metering
- peak meters
- RMS/LUFS-ready architecture
- plugin chains
- plugin bypass
- plugin latency
- PDC
- automation
- track naming
- track coloring

Routing must support arbitrary graph-like signal flow where safe.

Avoid hardcoded linear signal chains.

---

# 12. PLUGIN HOST

Plugin hosting is a major subsystem.

Design for:

- VST3
- Audio Units on macOS
- CLAP if practical
- future plugin standards

Support:

- plugin scanning
- plugin discovery
- plugin categorization
- plugin loading
- plugin unloading
- plugin state
- preset state
- automation
- parameter discovery
- parameter mapping
- plugin UI
- editor embedding
- offline processing
- realtime processing
- latency reporting
- crash isolation strategy
- plugin sandbox strategy
- plugin validation
- plugin blacklist
- plugin rescan

Never assume third-party plugins are safe.

The host must be resilient to plugin crashes and invalid behavior.

---

# 13. BUILT-IN INSTRUMENT ARCHITECTURE

INCDAW must eventually contain an extensible instrument framework.

Potential instruments:

- sampler
- subtractive synth
- wavetable synth
- FM synth
- granular synth
- drum machine
- multisampler
- rompler
- physical-modeling-ready architecture
- modular synthesis framework

Do not attempt to implement all instruments at once.

First build the instrument API.

---

# 14. BUILT-IN EFFECT ARCHITECTURE

Effect framework should support:

- EQ
- compressor
- limiter
- gate
- expander
- saturation
- distortion
- waveshaper
- reverb
- delay
- chorus
- flanger
- phaser
- stereo tools
- filters
- transient shaping
- de-esser
- utility gain
- analyzer
- spectrum analyzer
- loudness meter

Effects must share a common DSP/plugin interface.

---

# 15. AUDIO EDITOR

Build an Edison-like dedicated audio editor eventually.

Support:

- waveform view
- zoom
- selection
- trim
- cut
- copy
- paste
- fade
- crossfade
- normalize
- gain
- reverse
- silence
- denoise
- noise profile
- spectral editing architecture
- resample
- pitch shifting
- time stretching
- markers
- regions
- loop selection
- audio analysis
- recording

---

# 16. TIME STRETCHING / PITCH

Must support a pluggable time/pitch processing architecture.

Requirements:

- realtime preview
- offline high-quality mode
- pitch shifting
- time stretching
- tempo synchronization
- transient preservation
- formant-aware architecture
- algorithm selection

Do not write a low-quality placeholder and consider the feature complete.

---

# 17. RECORDING

Support:

- microphone input
- line input
- instrument input
- multiple simultaneous inputs
- input monitoring
- latency compensation
- recording modes
- loop recording
- take management
- comping-ready architecture
- punch recording
- pre-recording buffer
- continuous background audio capture architecture
- recording into playlist
- recording into audio editor

---

# 18. SAMPLER

Support:

- sample loading
- waveform
- start/end
- loop points
- crossfade loop
- root note
- key range
- velocity mapping
- pitch
- time stretch
- reverse
- envelopes
- filters
- LFO
- modulation
- sample layering
- multisampling

---

# 19. BROWSER

Browser must support:

- samples
- instruments
- presets
- plugins
- projects
- folders
- favorites
- tags
- search
- preview
- drag/drop
- metadata
- recent items
- user libraries

---

# 20. CONTENT / SOUND LIBRARY

Design a content abstraction for:

- samples
- loops
- one-shots
- presets
- MIDI
- project templates

Do not bundle copyrighted commercial content.

Use original/demo/public-domain assets where needed.

---

# 21. MIDI CONTROLLERS

Support:

- keyboard input
- MIDI controllers
- knobs
- faders
- pads
- transport controls
- MIDI mapping
- learn mode
- custom mappings
- hardware feedback
- MIDI clock
- synchronization

---

# 22. CONTROLLER LINKING

Create a generic parameter mapping system.

Any supported hardware/software control should be able to map to:

- mixer parameters
- instrument parameters
- plugin parameters
- transport
- automation
- macros

---

# 23. UNDO / REDO

Undo/redo is mandatory across the application.

Must support:

- project edits
- MIDI edits
- audio edits
- automation
- mixer changes
- routing
- plugin parameter changes
- UI state where appropriate

Prefer a command-based architecture.

Avoid ad-hoc undo implementations.

---

# 24. CLIP / PROJECT DATA MODEL

The internal data model must distinguish:

- project
- song
- timeline
- track
- clip
- pattern
- channel
- instrument
- plugin
- mixer node
- automation lane
- MIDI event
- audio asset
- preset
- routing connection

Do not collapse these into one generic object.

---

# 25. UI ARCHITECTURE

UI should be modular.

Main workspaces:

- Main Toolbar
- Playlist
- Piano Roll
- Channel Rack
- Mixer
- Browser
- Plugin windows
- Audio Editor
- Project settings
- MIDI settings
- Audio settings

Windows/panels should be dockable where practical.

Support:

- resizable panels
- scalable UI
- keyboard shortcuts
- context menus
- toolbars
- customizable layout
- workspace persistence
- dark professional interface
- accessibility-ready architecture

Do not copy FL Studio's exact visual design.

Create INCDAW's own identity.

---

# 26. KEYBOARD SHORTCUT SYSTEM

Create a centralized command registry.

Every action should be represented as a command.

Commands must support:

- keyboard shortcuts
- menu invocation
- toolbar invocation
- scripting-ready architecture
- command search

Avoid hardcoding shortcuts inside UI components.

---

# 27. PERFORMANCE

Performance is a feature.

Measure:

- audio callback duration
- UI frame time
- CPU load
- memory
- plugin load
- disk streaming
- project loading time
- waveform rendering
- Piano Roll rendering
- Playlist rendering

Use profiling before optimizing.

Do not optimize based on assumptions.

---

# 28. TESTING

Every core subsystem needs automated tests.

Priority:

1. audio engine
2. transport
3. MIDI
4. project serialization
5. automation
6. routing
7. plugin host
8. DSP
9. UI state
10. rendering/export

Add regression tests for every serious bug.

---

# 29. AUDIO QUALITY

Audio correctness is non-negotiable.

Test:

- sample-accurate timing
- clipping
- denormals
- NaN
- infinity
- channel mismatches
- sample-rate changes
- buffer size changes
- latency
- plugin latency
- automation interpolation
- transport seeking
- looping
- offline render equivalence

---

# 30. FILE FORMATS

Eventually support:

Audio:

- WAV
- AIFF
- FLAC
- MP3 where licensing/platform support allows

MIDI:

- Standard MIDI File import/export

Project:

- native INCDAW project format
- versioned schema
- migration system

Export:

- WAV
- FLAC
- MP3 where appropriate
- stems
- selected regions
- master
- mixer tracks

---

# 31. OFFLINE RENDER ENGINE

Create a dedicated render pipeline.

Support:

- master render
- stems
- individual tracks
- regions
- selected clips
- tail handling
- normalization
- sample rate conversion
- bit depth
- dither
- plugin offline processing

Realtime and offline processing must use compatible DSP paths where possible.

---

# 32. PROJECT ARCHITECTURE

Recommended high-level separation:

/apps
/core
/audio
/dsp
/midi
/timeline
/project
/plugins
/instruments
/effects
/mixer
/automation
/editor
/ui
/browser
/platform
/render
/tests
/docs
/tools

Do not blindly use this structure.

Use Graphify + actual repository state to determine the final structure.

---

# 33. PLATFORM

Target platforms should eventually include:

- macOS
- Windows

Architecture should not make Linux impossible, even if it is not a first release target.

Platform-specific code must remain isolated.

---

# 34. ENGINEERING RULES

Never:

- duplicate business logic
- duplicate audio logic
- create giant God classes
- create circular dependencies
- couple UI directly to DSP
- couple DSP directly to UI
- put project serialization inside UI
- put routing logic inside visual components
- use global mutable state without architectural justification
- create temporary hacks without documenting them

Prefer:

- interfaces
- dependency inversion
- event systems
- command systems
- immutable data where useful
- deterministic state transitions
- modular DSP
- clear ownership
- explicit lifecycle management

---

# 35. DOCUMENTATION

The project must continuously maintain:

- ARCHITECTURE.md
- ROADMAP.md
- DECISIONS.md
- REQUIREMENTS.md
- AUDIO_ENGINE.md
- PLUGIN_HOST.md
- PROJECT_FORMAT.md
- TESTING.md
- PERFORMANCE.md
- CHANGELOG.md

Do not allow documentation to become disconnected from implementation.

---

# 36. DECISION LOG

Every major architectural decision must be recorded.

Format:

Decision:
Context:
Options:
Chosen:
Reason:
Tradeoffs:
Date:
Status:

Never silently replace an architectural decision.

---

# 37. DEVELOPMENT PHASES

Do not implement the entire DAW at once.

Use phases.

Phase 0:
Research + architecture

Phase 1:
Foundation + build system

Phase 2:
Audio engine

Phase 3:
Transport

Phase 4:
Project model

Phase 5:
MIDI engine

Phase 6:
Piano Roll

Phase 7:
Channel/instrument system

Phase 8:
Pattern system

Phase 9:
Playlist

Phase 10:
Mixer/routing

Phase 11:
Automation

Phase 12:
Audio recording/editor

Phase 13:
Plugin hosting

Phase 14:
Sampler

Phase 15:
Built-in DSP

Phase 16:
MIDI hardware

Phase 17:
Rendering/export

Phase 18:
Performance

Phase 19:
QA

Phase 20:
Release engineering

Do not skip phases merely to make a demo look complete.

---

# 38. REQUIRED WORKFLOW FOR EVERY FEATURE

When the user asks for a feature:

## STEP A — GRAPH

Use Graphify.

Determine:

- related components
- dependencies
- existing abstractions
- possible conflicts
- architectural impact

## STEP B — RESEARCH

Research the relevant functional behavior.

For FL Studio reference behavior, prefer official Image-Line documentation.

Do not assume behavior from memory.

## STEP C — CURRENT STATE

Inspect the repository.

Identify:

- existing implementation
- missing pieces
- tests
- architecture
- technical constraints

## STEP D — PLAN

Produce:

### Objective

### Current state

### Proposed behavior

### Architecture

### Files/components affected

### API changes

### Data model changes

### Audio implications

### UI implications

### Performance implications

### Tests

### Risks

### Rollback strategy

### Definition of done

## STEP E — STOP

DO NOT IMPLEMENT.

Wait for explicit user approval.

---

# 39. IMPLEMENTATION AFTER APPROVAL

Only after approval:

1. Re-check Graphify.
2. Implement the smallest coherent increment.
3. Run relevant tests.
4. Build.
5. Run static analysis.
6. Run audio correctness checks where applicable.
7. Update documentation.
8. Update project graph.
9. Report exactly what changed.

Never silently expand scope.

---

# 40. SCOPE CONTROL

If implementation reveals another problem:

STOP.

Do not automatically fix unrelated problems.

Report:

"Scope expansion detected."

Then explain:

- discovered issue
- why it matters
- proposed fix
- affected components
- whether it blocks the approved task

Wait for approval.

---

# 41. DEPENDENCY POLICY

Never add a dependency without approval.

For every proposed dependency provide:

- name
- purpose
- license
- maintenance status
- platform support
- performance impact
- security implications
- alternatives
- reason for choosing it

Then STOP.

---

# 42. SECURITY

Never execute:

- destructive shell commands
- arbitrary downloaded scripts
- untrusted binaries
- unknown installers

without explicit user approval.

Do not download proprietary software or copyrighted assets.

---

# 43. LEGAL / IP BOUNDARY

INCDAW can implement equivalent functionality.

It must NOT copy proprietary implementation.

Use functional analysis of FL Studio as reference.

Do not use:

- FL Studio source code
- reverse-engineered proprietary internals
- copyrighted assets
- proprietary presets
- trademarked UI assets

INCDAW must remain independently implemented.

---

# 44. DEFINITION OF "DONE"

A feature is NOT done because:

- UI exists
- button exists
- code compiles
- mock data works

A feature is done only when:

- behavior is implemented
- architecture is correct
- tests exist
- edge cases are handled
- performance is acceptable
- documentation is updated
- Graphify reflects the architecture
- relevant regression tests pass

---

# 45. FIRST TASK

When entering a fresh INCDAW repository, DO NOT START CODING.

The first task is:

"Perform complete INCDAW discovery and architecture planning."

Do this:

1. Verify Graphify.
2. Run `/graphify .`
3. Inspect Graphify report.
4. Determine repository state.
5. Determine available build tools.
6. Determine platform strategy.
7. Determine current source tree.
8. Research current FL Studio functional areas from official documentation.
9. Identify the minimum viable architecture for a professional DAW.
10. Identify unknowns.
11. Identify risks.
12. Create a proposed roadmap.
13. Create a proposed architecture.
14. Create a dependency strategy.
15. Create a testing strategy.
16. Create a performance strategy.
17. Create a plugin strategy.
18. Create a project-format strategy.
19. Create a migration strategy.
20. STOP.

Do NOT create the implementation.

---

# 46. REQUIRED RESPONSE FORMAT

For planning tasks:

# INCDAW PLAN

## Understanding
...

## Graphify Findings
...

## Current Architecture
...

## Proposed Architecture
...

## Functional Scope
...

## Implementation Phases
...

## Risks
...

## Dependencies
...

## Testing
...

## Performance
...

## Open Questions
...

## Approval Required

"Nothing has been implemented. Awaiting explicit approval."

For implementation tasks:

# INCDAW IMPLEMENTATION REPORT

## Approved Scope
...

## Implemented
...

## Files Changed
...

## Tests
...

## Build
...

## Graphify Update
...

## Remaining Issues
...

## Scope Expansion
...

---

# 47. USER COMMUNICATION

The user does not need to understand internal engineering details.

Be concise but technically precise.

Do not hide risks.

Do not pretend something works when it does not.

Do not claim tests passed unless they actually ran.

Do not claim Graphify was used unless it actually was used.

Do not claim FL Studio parity unless the relevant feature has actually been implemented and tested.

---

# 48. CORE PRINCIPLE

INCDAW is not built by generating thousands of files.

INCDAW is built by creating a correct audio architecture first and progressively adding professional workflows on top of it.

PLAN FIRST.

GRAPHIFY FIRST.

ASK FOR APPROVAL.

THEN IMPLEMENT.

VERIFY EVERYTHING.
