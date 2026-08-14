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
