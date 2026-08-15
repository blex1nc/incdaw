# INCDAW — Architecture

Status: **Phase 0 — approved, not implemented.**
No source code exists yet. This document defines the target structure that
Phase 1 onward must build toward. It is binding: deviations require a new entry
in docs/DECISIONS.md.

---

## 1. Guiding principle

INCDAW is built as a **headless, deterministic, framework-free core** with thin
platform and UI shells around it. The core must compile and pass its entire test
suite with no UI, no CoreAudio, and no plugin SDK linked in. Anything that
cannot be tested headlessly does not belong in the core.

This is what makes the project survivable over years: the parts that are hard to
get right (timing, routing, serialization, automation) are also the parts that
are cheapest to test, and they never depend on the parts that are expensive to
test.

---

## 2. Layer model

Dependencies point **downward only**. A layer may use anything below it and must
know nothing about anything above it.

```
┌──────────────────────────────────────────────────────────┐
│  ui/          AppKit shell · Metal widget layer          │  may block, may
│               Playlist · Piano Roll · Mixer · Browser    │  crash, may be slow
├──────────────────────────────────────────────────────────┤
│  app/         command registry · undo stack · session    │
├──────────────────────────────────────────────────────────┤
│  project/     serialization · versioning · migration     │
│               asset management · relinking               │
├──────────────────────────────────────────────────────────┤
│  engine/      graph compiler · transport · scheduler     │  realtime
│               PDC · offline render                       │  boundary
│    ├─ midi/       events · tempo map · quantize          │
│    ├─ mixer/      routing DAG · tracks · sends · buses   │
│    ├─ automation/ generic parameter automation           │
│    └─ dsp/        realtime-safe primitives               │  NEVER allocates
├──────────────────────────────────────────────────────────┤
│  plugins/     scanner · registry · instance · params     │
│               state · UI bridge · isolation              │
├──────────────────────────────────────────────────────────┤
│  platform/    CoreAudio · CoreMIDI · workgroups · files  │  ONLY macOS code
└──────────────────────────────────────────────────────────┘
```

### Enforced rules

| Rule | Enforcement |
|---|---|
| `engine/` contains no reference to `ui/` or `app/` | Automated layering test, Phase 1 |
| macOS/platform symbols appear only in `platform/` | Automated layering test, Phase 1 |
| `dsp/` performs no allocation, locking, or I/O | Realtime guard in debug builds, Phase 2 |
| No circular dependencies between layers | Layering test |
| Serialization lives in `project/`, never in `ui/` | Layering test + review |

The layering test is written in **Phase 1, before any engine code**, so that the
rules are load-bearing from the first commit rather than aspirational.

---

## 3. Proposed repository structure

Only `docs/` exists today. The rest is created incrementally, one phase at a time.

```
INCDAW X/
├── CLAUDE.md               project constitution
├── HANDOFF.md              cross-session state
├── CHANGELOG.md
├── CMakeLists.txt          (Phase 1)
├── docs/
│   ├── ARCHITECTURE.md     this file
│   ├── REQUIREMENTS.md
│   ├── ROADMAP.md
│   ├── DECISIONS.md
│   ├── AUDIO_ENGINE.md
│   ├── PLUGIN_HOST.md
│   ├── PROJECT_FORMAT.md
│   ├── TESTING.md
│   └── PERFORMANCE.md
├── src/                    (Phase 1)
│   ├── platform/
│   ├── plugins/
│   ├── engine/{dsp,midi,mixer,automation}/
│   ├── project/
│   ├── app/
│   └── ui/
├── tests/                  (Phase 1)
│   ├── unit/
│   ├── integration/
│   ├── golden/             reference audio for regression
│   └── fixtures/           project files, one per format version
├── tools/                  build, packaging, DMG scripts
└── graphify-out/           knowledge graph (derived)
```

---

## 4. Threading model

Four thread classes. Crossing between them is only ever done by the mechanisms
listed.

| Thread | Responsibility | May allocate? | May block? |
|---|---|---|---|
| **Audio (RT)** | Device callback: render one buffer | **No** | **No** |
| **Worker (RT)** | Parallel graph nodes, joined to the audio workgroup | **No** | **No** |
| **UI** | Rendering, input, plugin editors | Yes | Yes |
| **Background** | Disk I/O, plugin scan, autosave, waveform generation, object reaping | Yes | Yes |

### Communication

- **UI → Audio:** lock-free SPSC command queue. Messages are POD or hold
  pre-allocated pointers. The audio thread never constructs anything.
- **Audio → UI:** lock-free SPSC queue for meters, playhead position, and
  notifications. The UI polls at frame rate; dropped messages are acceptable
  for anything the UI can re-derive.
- **Audio → Background:** objects the audio thread stops using are pushed to a
  reaper queue and destroyed off-thread. **The audio thread never calls a
  destructor that frees memory.**
- **Never:** mutexes shared between the audio thread and any other thread;
  `std::shared_ptr` refcount traffic on the audio thread; any form of "just this
  once" blocking.

The graph the audio thread reads is **immutable while it is being read**. Edits
build a new graph on a background thread and hand it over by atomic pointer
swap; the old graph goes to the reaper.

---

## 5. Data model

The constitution (§24) forbids collapsing these into one generic object. Each is
a distinct type with its own identity and lifetime.

```
Project
 ├── metadata, format version, tempo map, time signature map
 ├── Song ──► Timeline ──► Track* ──► Clip*
 │                                     ├── AudioClip   → AudioAsset
 │                                     ├── PatternClip → Pattern
 │                                     └── AutomationClip
 ├── Pattern*        MIDI events, automation events, reusable
 ├── Channel*        instrument / sampler / audio source / MIDI source
 ├── Instrument*     built-in or hosted plugin
 ├── MixerNode*      insert chain, sends, routing
 ├── RoutingConnection*   arbitrary graph edges, cycle-checked
 ├── AutomationLane*
 └── AudioAsset*     referenced or embedded media
```

**Identity:** every persistent entity carries a stable 64-bit id, unique within
the project and never reused. References between entities are by id, never by
pointer or index — this is what makes undo, serialization, and relinking
tractable.

**Ownership:** the `Project` owns everything. The engine holds a compiled,
read-only *view* of the project; it never owns model objects and never mutates
them.

---

## 6. Command architecture

**Every user-visible action is a command object.** No exceptions.

```
Command
 ├── id           stable name, e.g. "transport.play", "pianoroll.quantize"
 ├── execute(Project&)
 ├── undo(Project&)
 └── metadata     display name, category, default shortcut
```

All commands are registered in a central `CommandRegistry`. The registry — not
the UI — owns shortcut bindings, menu entries, and toolbar actions. UI
components invoke commands by id; they never mutate the project directly.

This single decision is what later makes possible, from one mechanism:
undo/redo, keyboard shortcuts, menus, macros, MIDI-learn mapping, command
search, and scripting. FL Studio's 2026 assistant can organise tracks, route
mixer channels, set levels and adjust plugin parameters precisely because FL
exposes such a surface; INCDAW gets the same capability for free by building
this way from the start, and cannot retrofit it cheaply if it does not.

**Undo** is a stack of executed commands. There is exactly one mutation path
into the project model: `CommandRegistry::execute()`. Anything that mutates the
model outside that path is a bug.

---

## 7. Engine boundary

The engine consumes a **compiled graph**, not the project model.

```
Project model  ──compile──►  RenderGraph  ──►  Audio thread
  (mutable,                  (immutable,        (reads only)
   UI thread)                 topologically
                              sorted, latency-
                              compensated)
```

Compilation happens on a background thread and performs: cycle detection,
topological sort, latency accumulation for PDC, buffer allocation, and node
parameter binding. The audio thread receives a finished object it can only read.

**Offline render uses the same `RenderGraph` and the same node implementations
as realtime playback.** This is not an optimisation — it is the only way to
guarantee that what you export is what you heard, and it is verified by an
automated byte-equivalence test (docs/TESTING.md).

### Where the compile lives

`project::compileProjectGraph` (Phase 8) is that arrow. It must live in
`project/`, because `engine/` sits below it and cannot see a `Project` — the
layering test enforces this. It produces, per audible channel, an
`InstrumentNode` fed by compiled notes and a `GainNode` carrying the channel
volume, summed into a master gain.

Everything musical is resolved during compilation, never on the audio thread:
polymetric repeats, swing, and note probability. Probability in particular is
rolled here from a seed derived from the project, so that a compile is
reproducible and an offline render produces the same notes as the playback the
user heard. The instrument itself is supplied by an injected
`InstrumentFactory`, so plugin hosting (Phase 13) becomes a different factory
rather than a change to the compiler.

Since Phase 10 the compiler also builds the mixer: every `MixerNode` becomes a
`MixerStripNode`, `RoutingConnection`s become edges (a send is an edge carrying
its own gain node), and each channel gets its own strip so that channel volume
and pan use the same arithmetic the mixer does. The signal path is
instrument → channel strip → mixer track → sends/buses → master, and a routing
cycle is refused by the graph builder rather than broken arbitrarily.

Since Phase 13 a mixer node's insert chain compiles in front of its strip, so
the full path through a track is inserts → fader → pan → mute → outputs. The
insert nodes themselves are built by an injected factory (D-028): the compiler
places `engine::Node`s and never learns what a plugin is, which is also what
lets Phase 15's built-in effects be inserts without being plugins.

### The editors above it

Every editing pane follows the same shape: a headless model holding geometry and
hit testing (`app::PianoRollModel`, `app::ChannelRackModel`), a view that turns
input into commands and a draw list into pixels, and no path to the project
model that does not go through `app::CommandRegistry`. The panes therefore share
one undo history rather than keeping their own, and a pane's arithmetic is
tested without a window.

The Channel Rack and the Piano Roll edit *the same notes*: a step is an ordinary
`MidiEvent` at the channel's `stepKey` (D-016). There is no step data type and
so nothing for the two views to disagree about.

Rendering is chosen per pane by measurement, not by policy: the Piano Roll is
Metal because it must cull ten thousand notes inside a frame, the rack, the
pattern list and the playlist are CoreGraphics because they are tens of
rectangles and mostly text (D-015).

### Patterns, tracks and the two transport modes

A channel makes sound; a track holds placements. They are deliberately not the
same axis, and CLAUDE.md §9 forbids assuming a 1:1 track-to-mixer relationship.
A pattern clip on a track plays whatever channels that pattern is programmed on,
so one clip can sound on eight channels and one channel can sound from clips on
several tracks.

Playback has two modes, and the difference is decided when the project is
compiled, never on the audio thread (D-017): pattern mode compiles one pattern
and loops it, song mode compiles the arrangement and loops its length. Track
mute and solo are resolved in the same pass (D-018), so a silenced track's notes
are never compiled rather than rendered and multiplied by zero.

---

## 8. Plugin isolation

Detailed in docs/PLUGIN_HOST.md. The architectural commitment made here: the
host ABI is designed for **out-of-process** hosting from its first line of code,
even though in-process hosting is implemented first. A crashing third-party
plugin must not be able to take down INCDAW or lose the user's project.
Retrofitting isolation into an in-process design means rewriting the host.

---

## 9. What this architecture deliberately does not do

- It does not assume a 1:1 track-to-mixer-channel relationship (CLAUDE.md §9).
- It does not build automation separately per plugin or control (§10) — there is
  one generic automation subsystem.
- It does not hardcode a linear signal chain (§11) — routing is a DAG.
- It does not put serialization in the UI, or routing logic in visual
  components (§34).
- It does not implement per-feature undo (§23) — undo is a property of the
  command system.
