# INCDAW — Decision Log

Every major architectural decision is recorded here, in the format required by
CLAUDE.md §36. Decisions are never silently replaced: superseding a decision
requires a new entry that references the old one and sets its status to
SUPERSEDED.

Status values: `PROPOSED` · `ACCEPTED` · `SUPERSEDED` · `REJECTED`

---

## D-001 — Core implementation language: C++20

**Context:** INCDAW requires realtime-safe audio processing, sample-accurate
scheduling, plugin delay compensation, and hosting of VST3/AU/CLAP plugins.
Plugin ABIs are C/C++ only. Garbage-collected and interpreted runtimes cannot
provide the deterministic, allocation-free audio callback the constitution
(CLAUDE.md §3) mandates.

**Options:**
- C++20 with clang
- Rust
- Swift
- JavaScript/TypeScript on Web Audio

**Chosen:** C++20, compiled with Apple clang 21.

**Reason:** It is the only ecosystem where every plugin SDK (CLAP, AU, VST3) is
natively consumable without an FFI shim, and it gives explicit control over
allocation, threading, and memory layout in the audio path. Rust is a credible
alternative but would require FFI wrappers for all three plugin formats and a
much smaller pool of prior art for DAW engines. Web Audio cannot host native
plugins, cannot control buffer size deterministically, and cannot implement PDC.

**Tradeoffs:** No memory safety guarantees from the language. Mitigated by the
testing strategy (docs/TESTING.md), the realtime-safety guard (docs/AUDIO_ENGINE.md),
and strict ownership rules in docs/ARCHITECTURE.md.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-002 — Build system: CMake + Ninja

**Context:** Neither CMake nor Ninja is installed on the development machine.
GNU Make 3.81 (2006, Apple-shipped) is present but too old to rely on. Full
Xcode is not installed — only Command Line Tools — so Xcode project generation
is not an available path.

**Options:**
- CMake + Ninja
- Meson + Ninja
- Bazel
- Hand-written Makefiles

**Chosen:** CMake (≥3.28) driving Ninja.

**Reason:** Works fully with Command Line Tools alone, requires no Xcode.app,
produces `compile_commands.json` for tooling, is the de-facto standard for
audio/plugin projects, and is the path of least resistance for the eventual
Windows target (D-005).

**Tradeoffs:** CMake's language is unpleasant. Accepted as the industry norm.
Both tools must be installed via Homebrew before Phase 1 can proceed — this is
a pending dependency approval, not yet granted.

**Date:** 2026-08-14
**Status:** ACCEPTED (installation of the tools themselves still pending approval)

---

## D-003 — Audio I/O: CoreAudio HAL directly, no wrapper framework

**Context:** The engine needs explicit control over buffer size, device safety
offset, latency reporting, sample-rate changes, and device hot-plug. Verified
during Phase 0 discovery: the macOS SDK at
`/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk` exposes CoreAudio,
AudioToolbox, AudioUnit, CoreMIDI, CoreAudioKit, Accelerate and Metal in full.

**Options:**
- CoreAudio HAL directly (`AudioDeviceIOProc`)
- AVAudioEngine
- A cross-platform wrapper (RtAudio, PortAudio, miniaudio)
- JUCE's device manager

**Chosen:** CoreAudio HAL directly, behind an INCDAW-owned `AudioDevice`
interface living in `platform/`.

**Reason:** AVAudioEngine imposes its own graph and node model, which conflicts
with INCDAW owning its signal graph. Wrapper libraries hide exactly the
properties (safety offset, workgroup handle, per-stream latency) that a DAW must
read to compute correct latency compensation. Every capability we need is
available natively, so a dependency would cost control without saving meaningful
work.

**Tradeoffs:** The Windows backend (WASAPI/ASIO) must be written separately.
This is acceptable because it sits behind one interface, and Windows is a later
target (D-005).

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-004 — Realtime thread scheduling: os_workgroup / Audio Workgroups

**Context:** The development and primary target machine is an Apple M5 with
heterogeneous P/E cores. Audio threads that are not joined to the device's
workgroup risk being scheduled onto efficiency cores, causing underruns.
Verified present in the SDK: `os/workgroup.h`, `os/workgroup_interval.h`,
`os/workgroup_parallel.h`, and `AudioToolbox/AudioWorkInterval.h` exposing
`AudioWorkIntervalCreate` and `kAudioDevicePropertyIOThreadOSWorkgroup`.

**Options:**
- Join the CoreAudio device workgroup via `kAudioDevicePropertyIOThreadOSWorkgroup`
- Use `pthread` time-constraint policy only (the older approach)
- Do nothing special

**Chosen:** Join the device workgroup; create an interval workgroup via
`AudioWorkIntervalCreate` for any auxiliary realtime worker threads.

**Reason:** This is Apple's sanctioned mechanism for realtime audio on Apple
Silicon. Independent corroboration: FL Studio 2026's official release notes
state that macOS "now uses Audio Workgroups API for audio thread scheduling,
reducing underrun risks." A shipping professional DAW reached the same
conclusion on the same hardware class.

**Tradeoffs:** macOS-specific; the Windows backend needs its own equivalent
(MMCSS "Pro Audio" task). Isolated in `platform/`.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-005 — Platform strategy: macOS first, Windows later, Linux not precluded

**Context:** User decision — INCDAW targets macOS now; Windows is a future
target; distribution is a `.dmg` for personal and team use.

**Chosen:** macOS/arm64 is the only build target for Phases 1–20. All
platform-specific code is confined to `platform/`. An automated layering test
fails the build if macOS symbols appear outside `platform/`.

**Reason:** Shipping one platform properly beats shipping two badly. The
isolation rule keeps the Windows port a matter of writing one new backend rather
than a rewrite, without paying for untested abstraction now.

**Tradeoffs:** The Windows backend stays unwritten and therefore unvalidated;
the abstraction may prove imperfect when it is finally exercised. Accepted
knowingly.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-006 — UI: AppKit shell + INCDAW-owned Metal-rendered widget layer

**Context:** The Piano Roll must stay at 60 fps with tens of thousands of notes;
the Playlist must handle hundreds of clips. No general-purpose widget toolkit
renders these well — every serious DAW draws its own editors regardless of
toolkit.

**Options:**
- AppKit shell + custom C++/Metal widget layer
- SwiftUI / AppKit native controls throughout
- A web frontend (Electron/Tauri-style) over IPC to the native engine
- JUCE's GUI layer

**Chosen:** A native AppKit window and menu bar hosting an INCDAW-owned,
GPU-accelerated widget layer written in C++ on Metal.

**Reason:** Since the two most important editors require custom rendering
anyway, adopting a toolkit would mean maintaining two rendering models. Owning
one gives consistent look, consistent input handling, and a single performance
model. Metal is verified present. A web frontend was considered — Node 26.5 and
pnpm are installed on this machine — but it adds an IPC boundary and a second
runtime for no benefit the engine can use, and it would make plugin editor
embedding substantially harder.

**Tradeoffs:** Significantly more work than adopting a toolkit; accessibility
must be built deliberately rather than inherited; native menus/dialogs still
require AppKit interop. Accepted as the cost of a professional DAW UI.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-007 — Plugin format support: CLAP, AU, VST3 (VST2 excluded)

**Context:** User decision — "support what FL Studio supports." Verified against
Image-Line's official Plugin Standards documentation: FL Studio on macOS
supports 64-bit VST 1/2, VST3, 64-bit Audio Units, and CLAP; on Windows, 32/64-bit
VST 1/2, VST3, CLAP, and Image-Line's own native format.

**Chosen:** Implement **CLAP → AU → VST3**, in that order. **VST2 is excluded.**

**Reason for the order:** CLAP is MIT-licensed, has the cleanest ABI, provides
sample-accurate events and an explicit host/plugin threading contract that
matches INCDAW's design — it is the best format on which to build the host
architecture. AU is next because it is native to macOS, mandatory there, and its
headers are already present in the SDK. VST3 is last but fully intended;
critically, VST3 SDK 3.8.x is now **MIT-licensed** (Steinberg withdrew the GPLv3
and proprietary options), which removes the licensing obstacle that would
otherwise conflict with D-008.

**Reason for excluding VST2:** Steinberg discontinued VST2 SDK licensing in 2018.
There is no lawful route for a new closed-source product to ship VST2 support.
This is a legal constraint, not a technical one, and it is the one place INCDAW
will deliberately not match FL Studio's format list.

**Tradeoffs:** Some older plugins will be unusable in INCDAW. Users can bridge
them with third-party VST2→VST3/CLAP wrappers if they choose.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-008 — Licensing: INCDAW is closed-source

**Context:** User decision. This constrains every dependency: no GPL, no AGPL,
and no copyleft transitively.

**Chosen:** Closed-source. Permissive dependencies only — MIT, BSD, Apache-2.0,
BSL-1.0, ISC, Zlib.

**Consequences:**
- **JUCE is rejected.** It is dual-licensed AGPLv3 / paid commercial
  subscription. The AGPL branch is incompatible with closed source, and the
  commercial branch is a recurring cost for a framework whose main benefits
  (device I/O, AU hosting) we already have natively via D-003 and D-007.
- VST3 SDK 3.8.x (MIT) and CLAP (MIT) are compatible.
- Every future dependency requires a license check recorded in this log before
  adoption, per CLAUDE.md §41.

**Tradeoffs:** No community contributions; all engine work is ours. Accepted.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-009 — Distribution: ad-hoc signed, un-notarized DMG

**Context:** User decision — no Apple Developer Program membership; INCDAW is for
personal and team use. Verified during discovery: `hdiutil`, `codesign`,
`notarytool`, `stapler`, `spctl`, `pkgbuild` and `productbuild` are all present
with Command Line Tools alone, but `security find-identity -v -p codesigning`
reports **0 valid identities**.

**Chosen:** Build `INCDAW.app`, **ad-hoc sign it** (`codesign -s -`), and package
it with `hdiutil` into an un-notarized `.dmg`.

**Reason:** Ad-hoc signing is not optional cosmetics — arm64 binaries will not
execute at all on Apple Silicon without at least an ad-hoc signature. Notarization
additionally requires a paid Developer ID, which is out of scope by decision.

**Consequences for team distribution:** macOS Gatekeeper will quarantine the DMG
when it is downloaded. Each team member must either right-click → Open on first
launch, or clear the quarantine attribute:

    xattr -dr com.apple.quarantine /Applications/INCDAW.app

This must be documented in the release notes for every build.

**Upgrade path:** If INCDAW is ever distributed publicly, enrolling in the Apple
Developer Program adds two steps (`codesign` with a Developer ID Application
certificate, then `notarytool submit --wait` and `stapler staple`) to the
existing pipeline. Nothing else changes — the pipeline is designed for this.

**Tradeoffs:** Friction on first launch for every team member. Accepted for a
non-public tool.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-010 — Version control: git, initialised at Phase 0

**Context:** The project had no repository at the start of Phase 0 — no history,
no branches, no rollback, no bisect. For a multi-year project this was the
highest-severity non-technical risk identified in discovery.

**Chosen:** `git init` on branch `main`, with a `.gitignore` covering macOS
noise, build output, machine-local Graphify state, and local tooling config.

**Reason:** Nothing else in the roadmap is safe without it.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-011 — Metal shaders are compiled at runtime, not built offline

**Context:** The Piano Roll renderer needs a Metal shader. The normal path is to
compile `.metal` files at build time with `xcrun metal` into a `.metallib`.
Verified on this machine: `xcrun --find metal` fails — the Metal offline
compiler ships with the full Xcode application, not with Command Line Tools,
and only Command Line Tools are installed (docs/DECISIONS.md D-002).

**Options:**
- Compile the shader source at runtime with `newLibraryWithSource:options:error:`
- Require a full Xcode installation and build a `.metallib` offline
- Avoid Metal and draw with Core Graphics

**Chosen:** Compile from an embedded source string at runtime, once, during
renderer initialisation.

**Reason:** It removes an 8 GB toolchain dependency from the build for what is,
at present, a forty-line shader. Compilation happens once at startup, off every
hot path, and the cost is not measurable against window creation. Core Graphics
was rejected because it cannot hold 60 fps with ten thousand notes, which is the
Phase 6 requirement.

**Tradeoffs:** Shader errors surface at launch rather than at build time; the
renderer therefore reports them explicitly and the view logs the failure instead
of presenting a silently empty editor. Startup pays a few milliseconds. If the
shader set grows substantially, or if a shipping build ever wants precompiled
pipelines, this decision should be revisited — it would then require Xcode.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-012 — A pattern stores its notes per channel

**Context:** Until Phase 8 a `Pattern` held one flat `std::vector<MidiEvent>`.
Phase 8 needs a Channel Rack and a step sequencer, both of which are views of
several channels programmed side by side inside the same pattern.

**Options:**
- Keep the flat list and put a channel id on every `MidiEvent`
- Give the pattern a list of per-channel content blocks
- Give each channel its own patterns, and make a "pattern" a set of them

**Chosen:** `Pattern::channels` — a vector of `PatternChannelContent`, each
holding a channel id, an optional loop length, and that channel's events.

**Reason:** It is the shape the workflow actually has. A flat list forces every
reader — compiler, editor, renderer, step sequencer — to filter by channel on
every pass, and leaves nowhere to put a per-channel property. The per-channel
loop length is what makes polymetric patterns expressible at all, and there is
no sensible place for it on an individual event. The third option was rejected
because it destroys the property that makes patterns useful: one pattern placed
in several places, edited once.

**Tradeoffs:** The project format changed (v1.0 → v1.1) and every note command
now addresses a (pattern, channel) pair rather than a pattern. Migration from
1.0 attaches the flat list to the project's first channel, creating one if the
project has none.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-013 — Pattern and automation clips are placed in ticks, audio clips in frames

**Context:** `Clip` stored `start`, `length` and `sourceOffset` in frames. A
pattern placed at bar 5 must stay at bar 5 when the tempo changes; a recorded
audio clip must stay where it was recorded.

**Options:**
- Frames everywhere, converting on tempo change
- Ticks everywhere, converting audio on the fly
- Both, with the clip type deciding which is authoritative

**Chosen:** Both. `startTick` / `lengthTicks` / `sourceOffsetTicks` are
authoritative for pattern and automation clips; `start` / `length` /
`sourceOffset` remain authoritative for audio clips.

**Reason:** The two clip kinds genuinely have different time bases, and
pretending otherwise pushes the problem into every consumer. Frames-everywhere
means the first tempo change silently desynchronises the whole arrangement —
recoverable only by rewriting every clip, which is a migration disguised as an
edit. Ticks-everywhere means an audio clip drifts against its own recording.

**Tradeoffs:** Two fields to keep coherent per clip, and code that touches
placement has to know which kind it is holding. Phase 9 will need a single
accessor that resolves placement by clip type rather than letting callers pick.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-014 — Swing displaces only notes exactly on the grid

**Context:** Swing shifts off-beat subdivisions later. Which notes count as
"on an off-beat" has to be decided: a note may sit a few ticks off a grid line
because it was played, nudged, or partially quantised.

**Options:**
- Displace every note, scaled by its position within the subdivision
- Displace notes within a tolerance window of an odd grid line
- Displace only notes exactly on an odd grid line

**Chosen:** Exactly on the line. A tolerance of `grid / 8` was implemented
first, and a test written against it — the test is what exposed the problem.

**Reason:** A tolerance width cannot be explained to the user, and it silently
decides that a note played 30 ms early was "meant" to be on the beat, which
destroys timing the performer intended. Steps and snapped Piano Roll notes land
exactly on the grid, so the common case is covered exactly; anything off the
grid was placed expressively and is left alone.

**Tradeoffs:** A part quantised at strength < 1 will not swing, because its
notes are deliberately not on the grid. That is the correct outcome — the user
asked for the timing to be preserved — but it means "quantise loosely, then
swing" is not a workflow INCDAW supports. If it is ever wanted, it belongs as an
explicit "swing as quantise target" operation, not as a tolerance here.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-015 — The Channel Rack is drawn with CoreGraphics, not Metal

**Context:** The Piano Roll is a Metal, layer-hosting view driven by a display
link, because docs/ROADMAP.md Phase 6 requires 60 fps with 10,000 notes. Phase
8b adds two more panes. The obvious move is to reuse the renderer.

**Options:**
- One Metal renderer shared by every pane
- CoreGraphics `drawRect:` for the rack and the pattern list
- An abstraction layer that hides which one is in use

**Chosen:** CoreGraphics for the rack and the pattern list; the Piano Roll keeps
Metal.

**Reason:** The two have genuinely different budgets. A rack draws a few
rectangles per channel per visible step, invalidated on edits and on the
playhead moving one cell — tens to low hundreds of rectangles, at rates a CPU
path handles without effort. Metal there costs a shader, a pipeline, a drawable
and a display link to save nothing measurable, and it makes text — channel
names, M/S, the pattern list — the hard part of a pane that is mostly text.

**Tradeoffs:** Two rendering paths in `ui/macos/`. The seam is per-view and
neither path is exposed outside its view, so a pane can switch later if a
measurement demands it. The rule is the measurement, not the API: a pane moves
to Metal when its frame time says so.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-016 — A step is a note, and its pitch belongs to the channel

**Context:** A step sequencer needs to know what a lit cell means. Most designs
give steps their own type — a bit per step, with parallel arrays for velocity
and probability — and convert to notes at playback.

**Options:**
- A dedicated step type, converted to MIDI when the pattern compiles
- Steps ARE notes: a lit cell is an ordinary `MidiEvent` in the pattern
- Steps as notes, but at one fixed pitch for every channel

**Chosen:** Steps are ordinary notes, written at a per-channel `Channel::stepKey`
(default 60). A cell is a half-open tick range, so a note nudged off the grid
still reads as programmed.

**Reason:** A second representation is a second source of truth, and the rack
and the Piano Roll edit the same pattern: with a step type, giving a step a
different length or probability in the Piano Roll would either be impossible or
would silently desynchronise the two views. Because a step is a note, every
existing note command, the compiler, undo and the project format apply to it
with no extra code. The pitch is per channel because a drum channel's steps have
to land on the key its sampler maps that drum to — a global constant would work
only until Phase 14.

**Tradeoffs:** "Is this step on?" is a search rather than a bit test, and a
channel programmed across many pitches shows only its step key in the rack. The
search is bounded by one channel's events in one pattern; if it ever shows up in
a profile, it is an index over an already-sorted list, not a data model change.
Project format 1.1 → 1.2 for the new field, additive and defaulted.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-017 — Pattern mode and song mode are a compile-time distinction

**Context:** A pattern-based DAW plays either one pattern on loop or the
arrangement. Phase 9a has to add the second without the first becoming a special
case of it.

**Options:**
- One graph that always plays the arrangement, with pattern mode as a temporary
  one-clip arrangement
- A transport flag the audio thread reads to choose between two note sources
- Two compilations of the same graph shape, selected when the graph is built

**Chosen:** The mode selects `PlaybackSource` when the project is compiled. The
audio thread has no idea there are two modes: it plays the notes the graph
carries, and the loop range comes from the pattern's length or from
`arrangementLengthTicks`.

**Reason:** A flag read on the audio thread means a branch in the hot path and a
race on every mode change. A synthetic one-clip arrangement means pattern mode
stops being the thing the user edits. Recompiling is already what every edit
does, and it costs 0.088 ms for an arrangement of 512 clips — far below the
threshold where a mode switch would feel like anything at all.

**Tradeoffs:** Switching mode while playing restarts from zero, because the
sequence the instrument nodes hold is replaced. FL restarts too; a mode change
that continued mid-bar from a different note source would be worse than a
restart, not better.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-018 — Track mute and solo are resolved when the arrangement compiles

**Context:** Clips sit on tracks, but sound comes from the channel a pattern is
programmed on. Track mute therefore cannot be a gain: there is no per-track node
to silence.

**Options:**
- Give every track a gain node in the graph and mute that
- Skip muted tracks' clips when the arrangement compiles
- Filter notes on the audio thread by track

**Chosen:** Skip them at compile time, exactly as channel mute already works.

**Reason:** A muted track's notes are then never compiled, never scheduled and
never rendered, rather than rendered and multiplied by zero. It also keeps the
graph shape independent of the arrangement: tracks are an organisational layer,
not a signal-path one, and inventing a node per track now would collide with the
real mixer in Phase 10.

**Tradeoffs:** Unmuting recompiles rather than flipping a gain, which is
measured and cheap. A muted track does not shorten the song —
`arrangementLengthTicks` counts what the user drew, so unmuting cannot move the
end of the song.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-019 — Delay compensation lives in the graph compiler

**Context:** Phase 10 must align parallel paths whose latencies differ. The
compensation could sit in `project/`, where the mixer is assembled, or in
`engine/`, where the graph is compiled.

**Options:**
- Compensate in `project::compileProjectGraph`, which knows what a mixer is
- Compensate in `engine::GraphBuilder::compile`, which knows the topology
- Compensate at runtime, reading each node's reported latency per block

**Chosen:** `GraphBuilder::compile`, which already computed the longest path to
every node in order to report total latency. It now acts on that analysis and
inserts `DelayLineNode`s on the short edges into any node that sums.

**Reason:** Alignment is a property of the graph, not of the mixer. Doing it in
the engine means playback, offline render, freeze, and every future plugin chain
get it without asking, and none of them can forget. Runtime compensation would
put a variable delay on the audio thread and make the correction depend on when
it was measured.

**Tradeoffs:** The compiler mutates the graph it was handed, which makes
`compile` less of a pure function; it is done once, before any rendering, and
`setDelayCompensationEnabled(false)` exists so the compensation can be tested
against its own absence. Delay lines cost memory proportional to the latency
being compensated, allocated in `prepare`.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-020 — A mixer strip is one node

**Context:** A strip sums its inputs and applies polarity, pan, volume, mute and
metering. Each of those could be a node in the graph.

**Options:**
- A chain of small nodes per strip, composable and individually testable
- One `MixerStripNode` doing all of it in a single pass
- Fold the strip into the mixer compiler as inline arithmetic

**Chosen:** One node.

**Reason:** A chain costs a buffer and an indirection per stage per block for
arithmetic that fits in one pass over the samples, and none of the stages is
independently useful — nobody wants a pan without a fader. Insert effects, which
*are* independently useful, chain in front of a strip rather than inside it,
which is what keeps them orderable and bypassable when Phase 13 and Phase 15
arrive.

**Tradeoffs:** The node has four parameters instead of one, and a future
surround pan law will land inside it rather than replacing a node. Measured: a
64-strip mixer costs 0.042 ms per 256-frame block, against a 5.33 ms budget.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-021 — Pan is constant power, centre is -3 dB

**Context:** Panning has to decide what "centre" means. A linear law keeps the
sum of the gains constant; a constant-power law keeps the sum of their squares
constant.

**Options:**
- Linear: centre at unity on both sides
- Constant power: centre at -3 dB on both sides
- Selectable per project

**Chosen:** Constant power, `cos`/`sin` of one angle, centre at 0.7071.

**Reason:** Power, not amplitude, is what the ear tracks for a source moving
across the image; a linear law makes a sound audibly louder in the middle of a
sweep. It is also what every console and every DAW's default does, so an
imported arrangement pans the way its author heard it.

**Tradeoffs:** Summing a hard-panned pair back to mono gives -3 dB rather than
unity, which surprises people who expect a balance control. Selectable laws are
a real feature (FL offers them), but a project-wide switch changes the meaning
of every existing pan value, so it needs the automation and project-migration
machinery to arrive with it rather than before it.

**Date:** 2026-08-14
**Status:** ACCEPTED

---

## D-022 — Automation is a registry plus a node, evaluated per block

**Context:** CLAUDE.md §10 requires one generic automation subsystem; the
roadmap's exit criterion is that a registered parameter is automatable with no
parameter-specific code. Something has to know what "volume" means, and
something has to run the envelopes.

**Options:**
- Evaluate on a UI timer, writing to the engine at 30 Hz
- Evaluate on the audio thread from a node compiled into the graph
- Give every parameter its own automation implementation as needed

**Chosen:** A `ParameterRegistry` (key → how a normalised value lands on a
strip) plus an `AutomationNode` compiled into the graph, evaluating every lane
once per block and writing through the same smoothed atomic setters the mixer's
fader uses.

**Reason:** A UI timer stops automating when the window is busy and knows
nothing of offline rendering; the node rides the graph's execution order, sees
the same `playPosition` the instruments see, and therefore renders offline
identically for free. Per-block resolution cannot click because every setter it
reaches is smoothed. The registry is the one place parameter meaning lives —
the exit-criterion test registers a key the codebase has never seen and
automates it, which is what proves nothing else needed to know.

**Tradeoffs:** Resolution is one value per block (~5 ms at 256/48k), which is
inaudible through the smoothers but is not sample-accurate; if a curve ever
needs audio-rate precision (an LFO-as-automation), that becomes a modulation
source in the instrument framework (Phase 15), not a faster automation lane.
Appliers are `std::function`s called on the audio thread — they must capture
only pointers owned by the same graph, which the compiler guarantees and the
header documents.

**Date:** 2026-08-15
**Status:** ACCEPTED


---

## D-023 — Capture is a second clock domain, reconciled by timestamps

**Context:** Phase 12 needs audio input. On Macs the microphone and the
speakers are separate HAL devices (AirPods split further: the mic is its own
device at its own rate), so "the device's input" does not exist in the common
case. Captured audio must land on the timeline where it actually happened,
which is the roadmap's loopback exit criterion.

**Options:**
- Require an aggregate device so input and output share one callback
- Open the input device separately, with its own IOProc, and reconcile by
  host-clock timestamps
- Resample capture against the output clock from day one

**Chosen:** A second IOProc on the input device (none when the user picks a
true duplex interface — then the main proc's input arguments are used). Blocks
cross the realtime boundary carrying the HAL's input timestamp; the recorder
subtracts the device's reported total input latency (buffer + safety offset +
stream latency) when reporting where the take starts. The capture path is an
atomic sink pointer on the engine (`AudioCaptureSink`), a lock-free sample
ring, and a polling writer thread draining to `WavStreamWriter`. A nominal
rate mismatch that the input device refuses to fix is a hard, explained
failure — not silent resampling.

**Reason:** Aggregates hide the clock problem without solving it and force
device setup on the user. Timestamps are the one currency every device already
pays in, and they are what MIDI input already uses, so capture aligns by the
same mechanism the rest of the engine trusts. The compensation is applied at
one edge and proven by the loopback tests, which also assert that removing it
misaligns by exactly the reported latency — the compensated pass cannot be
passing by accident.

**Tradeoffs:** Two devices genuinely drift (measured in samples per minutes);
within one take this is second-order and unhandled — drift correction (slow
resampling against the output clock) is future work, recorded here so nobody
mistakes its absence for a bug elsewhere. Capture-side sample-rate conversion
is refused rather than implemented, which surfaced immediately on hardware:
an AirPods mic in HFP mode offers 24 kHz and is rejected with an error naming
both rates. The writer thread polls (2 ms) rather than being signalled, which
costs nothing measurable and keeps even a wait-free syscall off the capture
callback.

**Date:** 2026-08-15
**Status:** ACCEPTED

---

## D-024 — A take is placed by clock correlation, not by counting

**Context:** A recorded take must land on the timeline exactly where the
musician heard themselves play. Phase 12 part 3 connects the recorder's
host-time take start to a timeline frame.

**Options:**
- Count blocks while recording and derive position from block arithmetic
- Latch the transport position when recording starts and trust it
- Publish a per-block (host time, timeline frame) anchor and map through it

**Chosen:** The engine publishes a `TimelineAnchor` from every rendered block
through a seqlock — the audio thread never blocks, readers retry the rare torn
read. `RecordingSession::finish` maps the take's latency-compensated start
through the freshest anchor: both sides ride the output device's clock, so the
linear extrapolation is exact.

**Reason:** Counting blocks breaks the moment a block is split by a loop or a
seek, and latching at arm time is wrong by however long arming preceded the
first captured sample. The anchor states the correlation the engine already
knows, once per block, and everything else is arithmetic. It is also the
mechanism 11b's automation recording will use unchanged.

**Tradeoffs:** The map is linear, so a seek or loop wrap DURING a take places
everything after it incorrectly; loop/punch recording needs per-segment
anchoring and is outstanding Phase 12 work. A take armed with the transport
stopped lands at the playhead instead — there is no moving timeline to
correlate against, and pretending otherwise would invent a position.

**Date:** 2026-08-15
**Status:** ACCEPTED

---

## D-025 — Streaming is a window, and starving it is audible, not fatal

**Context:** Phase 12 part 4. Audio clips were preloaded whole at graph
compile time, which is right for takes and wrong for an hour of audio: memory,
and project-load time. Streamed playback needs disk I/O that can never touch
the audio thread.

**Options:**
- A ring buffer per stream with seek negotiation between the two sides
- A double-buffered window: two segments leapfrogging ahead of the play
  position, each guarded by a seqlock
- Preload everything and cap project audio length

**Chosen:** The window. Each streamed clip owns two segments (~1.4 s each at
the default size); the audio thread's read publishes the position it wanted,
and a service thread refills whichever segment no longer covers what comes
next. Reads that the window cannot serve are zero-filled and counted — the
same honesty contract as the recorder's ring. One stream PER CLIP, not per
asset: two clips of one file at different positions would fight over a shared
window and starve each other. The compiler decides preload-versus-stream from
the header alone (`streamingThresholdFrames`, default 30 s), prefills the
window at compile time so a rebuilt graph starts warm, and the application
owns one `DiskStreamer` whose weak references let streams die with their
graphs. Seeks are not a protocol: they are just a requested position the
window does not cover yet, served one service-pass later.

**Reason:** A seek-negotiating ring is more machinery for the same guarantee;
the window makes the invariant visible — at most two contiguous spans exist,
and either a span covers the request or silence is the answer. Equivalence is
provable and proven: the streamed path renders bit-identically to the
preloaded path in tests, with tiny segments forcing refills mid-play.

**Tradeoffs:** A mid-play jump costs one window of silence (a few
milliseconds of service latency) before audio resumes; pre-buffering around
known jump targets (loop points) is future work alongside loop recording.
Per-clip windows cost ~1 MB of memory each — a hundred streamed clips is
100 MB, acceptable, and clips under the threshold never pay it.

**Date:** 2026-08-15
**Status:** ACCEPTED

---

## D-026 — Automation placement is a window over a lane

**Context:** Phase 11b. 11a's lanes played everywhere; automation clips,
pattern-scoped automation, and recorded passes all need automation that
plays somewhere specific.

**Options:**
- Bake windows into the point data at compile time
- Give each engine binding a tick window and let placement be data
- A separate windowed-automation node type

**Chosen:** `AutomationNode::Binding` carries a half-open tick window and
simply does not evaluate outside it. The compiler turns every placement — an
automation clip, or a pattern clip whose pattern lists lanes — into one
windowed binding with the lane's points shifted to position. A lane placed
anywhere plays only through its placements (a muted placement still counts
as placed); only an unplaced lane plays globally, which is 11a's behaviour
and what a freshly recorded pass does until it is arranged.

**Reason:** Not writing outside the window gives clip semantics for free:
nothing before the clip starts, and the value it last wrote holds after it
ends — the parameter is a strip the fader also owns, and silence from the
automation side means the fader's value stands. Baking windows into points
cannot express "do not touch", and a second node type would duplicate the
evaluator for a two-field difference.

**Tradeoffs:** Overlapping placements of the same lane both write; the later
binding in compile order wins within a block. Write-mode recording restarts
its stream on a loop wrap rather than overdubbing — loop-aware overdub is
latch-mode work, recorded as deferred along with touch mode and a dedicated
point-editing surface.

**Date:** 2026-08-15
**Status:** ACCEPTED

---

## D-027 — CLAP SDK vendored, pinned at 1.2.6

**Context:** Phase 13 needs the CLAP C ABI headers. CLAUDE.md §41 requires a
recorded rationale for every dependency; the user explicitly waived the
pre-approval presentation and authorised continuous execution through the
remaining phases, which covers this vendoring.

**The dependency:** free-audio/clap, tag 1.2.6, header-only C ABI, MIT
licensed (license copied alongside). Vendored under `third_party/clap/`,
gitignored like doctest; a fresh clone refetches with
`git clone --depth 1 --branch 1.2.6 https://github.com/free-audio/clap` and
copies `include/clap`. Maintained by the CLAP consortium (Bitwig, u-he et
al.); zero runtime cost — headers only; no security surface of its own (the
plugins it loads are the surface, which is what the isolation strategy is
for). Alternatives: none — this IS the format's canonical definition.

**Chosen order stands (D-007):** CLAP first — cleanest ABI, no SDK build
system, MIT; AU next (native, no vendoring needed); VST3 after.

**Date:** 2026-08-15
**Status:** ACCEPTED

---

## D-028 — Hosted plugins reach the graph through an injected factory

**Context:** the graph compiler lives in `project/` because it needs the
project model, and `engine/` sits below it. Insert effects are hosted
plugins, and everything about hosting one lives in `plugins/`, whose headers
carry CLAP types. Compiling a strip's insert chain therefore looked like it
required `project/` to include a plugin header — which would have put the
CLAP C ABI in the include path of the serializer, the pattern compiler and
every test that touches a project.

**Options:**

1. `project/` links and includes `plugins/`. Simplest; drags the plugin host
   into every consumer of the project model, and makes a headless render tool
   depend on a plugin ABI it may never use.
2. Move insert compilation into `plugins/`. Splits one topology across two
   layers; the compiler still has to know where the chain attaches.
3. Inject the insert node factory into `GraphCompileOptions`, the way
   `InstrumentFactory` already is. The compiler builds the topology and asks
   for an `engine::Node` per slot; the shell supplies a factory backed by
   `plugins::PluginInstanceManager`.

**Chosen:** option 3.

**Reason:** it is the pattern already established for instruments, and it
makes the compiler's contract honest — an insert is *a node*, not *a plugin*.
Built-in effects (Phase 15) will be inserts too, and they are not plugins at
all. A factory that returns nullptr yields a pass-through slot and a compile
warning, so a missing plugin costs its own slot and never the rest of the mix.

**Tradeoffs:** the shell has to wire the factory (and link `incdaw_plugins`),
and a caller that forgets gets a project whose inserts are silent — which is
why the compiler emits a warning naming each skipped slot rather than
dropping it quietly. The `std::function` indirection is paid once per slot at
compile time, never on the audio thread.

**Also decided here:** inserts run **pre-fader**. Everything reaching a strip
is summed by the first insert, passed down the chain, and only then scaled by
volume, panned, inverted and muted. A fader move must not change what a
compressor hears. `tests/unit/PluginInsertTests.cpp` proves the ordering with
a non-linear insert, and the test fails if the chain is wired after the strip.

**Date:** 2026-08-15
**Status:** ACCEPTED

## D-029 — Plugin parameters automate through a sink target and an event queue

**Context:** the ParameterRegistry's entries bound a normalised value onto a
`MixerStripNode&`, which was the only automatable target that existed. A
hosted plugin's parameter has a different target — a live instance — and a
different delivery contract: CLAP forbids calling into the plugin while it is
processing, so parameter changes travel as `clap_event_param_value` events in
the process call's input list (or through `params->flush()` when idle), never
through a setter. The roadmap's Phase 11 exit criterion — a registered
parameter is automatable with no parameter-specific code anywhere else — must
survive both facts.

**Options:**

1. A second, plugin-specific automation path beside the registry. Forbidden
   outright by CLAUDE.md §10, and it would fork every downstream feature
   (recording, MIDI learn) into two implementations.
2. Make the registry's applier target a base class both strips and plugins
   implement. Uniform, but it forces MixerStripNode to speak a generic
   parameter-id protocol it does not have, and renumbering strip setters into
   ids would trade compile-time safety for none.
3. Generalise the ENTRY: `Entry::apply` becomes a variant of two applier
   kinds — `StripApplier` (unchanged) and `SinkApplier` over a new pure
   interface `engine::ParameterSink`. Which alternative an entry holds tells
   the compiler what to resolve the lane's target to: the strip rendering the
   entity, or the sink of the insert slot the entity names. `ClapInstance`
   implements the sink by pushing onto a preallocated lock-free queue that
   its own `process()` drains into the block's CLAP input event list.

**Chosen:** option 3.

**Reason:** the registry stays the single place where per-parameter knowledge
lives, and the AutomationNode still knows nothing about what it automates.
The sink interface carries PLAIN values (the parameter's own range) because
that is what CLAP events carry; the normalised-to-plain mapping is applier
code, generated generically from discovery (`registerPluginParameters`).
Registry keys are scoped per plugin TYPE (`plugin:<uid>:<param-id>`); which
INSTANCE a lane drives is its `targetEntity` — the insert slot id — so two
instances of one plugin share entries. Node exposes an optional
`parameterSink()` accessor, so the compiler binds sinks without the insert
factory's signature changing and without knowing what a CLAP is (D-028
holds).

**Tradeoffs:** delivery is per-block (`time = 0`), because the AutomationNode
evaluates once per block — a plugin that does not smooth its own parameters
can zipper under fast automation; sample-accurate splitting can arrive later
without changing the architecture. The host does not smooth plugin
parameters (unlike strip setters): smoothing plain values in the host would
double-smooth well-behaved plugins and misbehave on stepped parameters. A
full queue drops values, healed one block later because automation writes
every block. `params->flush()` is NOT yet called when the engine is idle —
a UI-originated change while no audio device runs will sit queued until
processing resumes; that lands with the editor/UI bridge (PLUGIN_HOST §7).

**Date:** 2026-08-15
**Status:** ACCEPTED

## D-030 — Plugin state travels as blob files inside the package

**Context:** PLUGIN_HOST §6 defines plugin state as an opaque blob the host
stores and never interprets. `PluginSlot::stateFile` has been in the model
and in project.json since the inserts serialized (so no format change was
pending), but nothing captured a live plugin's state or handed it back. The
open questions were where blobs live, who reaches a live instance at save
time, and what happens when a plugin misbehaves or is missing.

**Options:**

1. Embed blobs base64-encoded inside project.json. One file, but it bloats
   the document, breaks the package's per-file corruption resilience, and
   makes a 50 MB sampler state re-write on every save.
2. Blob files under `plugins/` in the package, referenced by the slot's
   `stateFile`; live instances reached the same way automation reaches them —
   a capability accessor (`Node::stateIO()`) surfaced per slot by the graph
   compiler.

**Chosen:** option 2.

**Reason:** it is the package's own design ("a corrupted pattern costs one
pattern") applied to plugins, and it makes the missing-plugin rule free: a
slot with no live instance is simply not touched, so its blob and stateFile
survive untouched until the plugin returns. `engine::StateIO` mirrors
D-029's ParameterSink exactly — a pure interface in engine/, implemented by
ClapInstance over CLAP_EXT_STATE with stack-local stream adapters, exposed
per slot on CompiledProjectGraph. `project/PluginStateFiles` owns the file
side: capture BEFORE ProjectFile::save (so stateFile lands in project.json),
restore after compile. Blob writes stage-and-rename like every package write,
so a crash mid-save loses the new blob, never the old one.

**Tradeoffs:** capture reads live instances, so it needs the compiled graph —
a project saved with no graph compiled keeps its previous blobs (correct,
since the user heard nothing new). A hostile plugin is held to a 64 MB save
cap and refused beyond it. Failures are warnings by design: a plugin that
will not save keeps its previous state; one that rejects its blob plays its
defaults, named to the UI. The application still lacks a save/open action, so
the shell does not yet call capture/restore — recorded as the standing
"largest gap" in HANDOFF; the library side is complete and tested.

**Date:** 2026-08-15
**Status:** ACCEPTED
