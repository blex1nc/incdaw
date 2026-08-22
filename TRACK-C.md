# TRACK C — "bridge" · hardware, hosting and files

Branch: `claude/bridge` · Worktree: `.claude/worktrees/bridge`

You are one of three parallel tracks closing INCDAW's gap against FL Studio.
The full re-audited gap list is `docs/FL2026_GAP.md` §3; this brief is your
slice of it. The other two tracks are `timbre` (instruments and effects) and
`arrange` (playlist and automation). You will not need their files.

## What this track is for

Everything where INCDAW meets something that is not INCDAW: MIDI hardware,
other people's plugins, other people's file formats, and the recorded audio a
session produces. It is the track with the most one-way doors in it, so it is
also the one with a gate on three of its items.

## Your files

**Yours, exclusively**

- `src/platform/` and `src/platform/macos/` — all of it
- `src/plugins/` — all of it, including `clap/` and `au/`
- `src/engine/midi/` — all of it
- `src/engine/audio/` — the recorder, the streams and the file readers/writers
- `src/project/OfflineRender.*`
- `src/app/commands/RecordingCommands.*`, `MidiMappingCommands.*`,
  `AudioEditCommands.*`
- `src/ui/macos/AudioEditorView.*`, `src/ui/macos/SettingsWindow.*`
- `tests/unit/MidiTests.cpp`, `MidiMappingTests.cpp`, `PluginHostTests.cpp`,
  `PluginRegistryTests.cpp`, `AudioUnitTests.cpp`, `AudioRecorderTests.cpp`,
  `LoopRecordingTests.cpp`, `AudioEditsTests.cpp`, `RenderGraphTests.cpp`,
  and any new test file

**Shared — additive only:** `src/CMakeLists.txt`, `tests/CMakeLists.txt`,
`CHANGELOG.md`, `src/ui/macos/main.mm` (settings wiring, menu items and window
plumbing for your features only).

**Not yours:** `src/project/Model.h` and `ProjectFile.cpp` (track B owns the
format — if a feature of yours needs a new field, **stop and report it**),
`src/engine/instrument/`, `src/engine/dsp/effects/`, `PlaylistView.*`.

## GATED ITEMS — read before starting C12–C14

Three items need a dependency and CLAUDE.md §41 forbids adding one without
approval. For each, produce a written proposal — name, purpose, licence,
maintenance, platform support, performance and security implications,
alternatives, and why this one — and then **STOP** and wait. Do not vendor,
download, or add a submodule before the answer comes back.

- **C12 VST3** — Steinberg's SDK is dual-licensed (GPLv3 or a proprietary
  agreement); the licence choice affects how INCDAW itself may be distributed.
  This is the user's decision, not an engineering one.
- **C13 FLAC** — libFLAC (BSD). Straightforward, still needs approval.
- **C14 MP3** — encoding needs LAME (LGPL) and carries patent history worth
  stating. Propose the zero-dependency alternative alongside it: macOS
  AudioToolbox already encodes AAC and ALAC through `AudioConverter`, which
  covers "a compressed file to send someone" without a new dependency.

Everything else in this track is ungated. Do the ungated work first.

## The work, in order

### C1 — MIDI output

`platform::MidiDevice` is input only. Give it the other direction: device
enumeration for outputs, a settings selector, an output port on the engine
side, and a channel that can be routed to an external instrument instead of a
builtin one. Sample-accurate scheduling against the transport, and the write
must be realtime-safe — the audio thread hands messages over, it does not call
CoreMIDI's blocking paths.

### C2 — MIDI clock and transport sync

Send: clock, start, stop, continue and song position pointer, derived from the
tempo map so a tempo change is followed rather than approximated. Receive: the
same, with the transport slaved to an external clock and a jitter filter that
does not chase every message. Settings picks the role — off, send, receive.

### C3 — controller feedback

The mapping system is one-way today: hardware moves a parameter and nothing
goes back. Add the return path so a motorised fader follows automation and a
pad lights when its step is programmed. It is the same map read backwards, and
it must not feed back on itself.

### C5 — device hot-plug

Settings rescans only when asked. Listen for CoreMIDI and CoreAudio device
notifications and re-enumerate, keeping the user's chosen device if it comes
back and saying so plainly if it does not.

### C4 — MPE input

The MIDI representation is described as MPE-ready but nothing decodes it: no
zone configuration, no per-note pitch bend, no per-note pressure or timbre
routed to a voice. Decode it at the input, carry it through the note
representation, and make the Sampler and the Reference Synth respond. Do not
edit the instruments — if a voice needs a per-note modulation input the
instrument does not have, **stop and report it** so track `timbre` adds it.

### C6/C7 — the audio editor grows an edit model

- **C6** cut, copy and paste between selections and between files, with the
  clipboard surviving a document switch.
- **C7** markers and regions inside the editor. The timeline has both already
  (`TimelineMarker`); the editor has neither, and region-based export and
  slicing both want them.

### C8/C9 — takes and comping

Loop recording stacks takes today and nothing comps them. Build the comping
editor: takes shown as lanes, a range assigned to a take by dragging, the
composite audible while editing, and one undo entry per assignment. **C9** is
the input-side pre-record buffer — the last N seconds of an armed input kept
so a take that was not being recorded can still be kept. The master-side
Audio Logger is the pattern, not the mechanism.

### C10/C11 — restoration and spectral

- **C10** denoise with a captured noise profile: select silence, learn, then
  subtract across the selection.
- **C11** a spectral view in the editor, then spectral editing — select a
  region in time and frequency and attenuate it. The FFT is in
  `engine/dsp/Fft.h`; the analyzer's `SpectrumView` is the drawing precedent.

### C16 — AU instrument hosting

AU effects host already; instruments are excluded at the enumeration step
(`isInstrument` is filtered out). Lift that: an AU instrument on a channel,
MIDI in, audio out, editor and state through the existing `HostedPlugin`
interface.

### C15 — out-of-process plugin sandboxing (last)

A crashing plugin takes the application with it. Plugin scanning already runs
in a child process (`platform::ChildProcess`) — extend that shape to hosting:
a plugin runs in a child, audio and events cross a shared-memory ring, and a
crash loses one plugin rather than the session. Write the plan first; this is
its own program and it is fine for it to be the last thing this track does.

## Definition of done for this track

Hardware features: verified against a real device where one exists, and
degrading clearly when none does. Hosting: a misbehaving plugin cannot take
the application down or corrupt a project. File formats: round-trip tests,
and an import that refuses a malformed file rather than trusting it.

---

## Working rules (identical for all three tracks)

**Your branch.** You work in this worktree, on this branch, and nowhere else.
Push after every commit — `git push origin <this branch>`, **never to `main`**.
The global "push to main after every patch" note does not apply here; main is
where the three tracks get merged, one at a time, by the user.

**Never stage by wildcard.** No `git add -A`, no `git add .`, no `git commit -a`.
Stage the exact paths you changed. Two sessions once shared one working tree in
this project and swept each other's half-written work into unrelated commits;
that is why this rule is written down.

**File boundaries.** Each track owns a disjoint part of the tree (below). Four
files are shared choke points and every track has to touch them:

- `src/CMakeLists.txt`, `tests/CMakeLists.txt` — add your source lines only,
  next to the neighbouring ones. Never reorder or reformat.
- `CHANGELOG.md` — add your entry under `## [Unreleased]`. Do not touch
  released sections.
- `src/ui/macos/main.mm` — the shell. Additive only: your menu item, your
  window, your command registration. Never restructure what is there.

If the work needs a file another track owns, **stop and report it** rather than
editing across the boundary. That is a merge conflict with a person's name on
it, not a code problem.

**The process still applies** (CLAUDE.md §38–§44). The scope in this brief is
pre-approved; individual items inside it are not a licence to skip the rest:

1. Consult the graph first — `graphify query "…"` — before reading files.
2. Smallest coherent increment, then build, then tests.
3. `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`
   `cmake --build build -j8` — the tree builds with `-Werror`; warnings are errors.
4. `./build/tests/incdaw_tests` — every case green before you commit.
5. New behaviour needs new tests. A DSP block needs a null test at its
   transparent settings and a reference implementation written independently
   of the class under test (see `tests/unit/BuiltinEffectTests.cpp`).
6. Realtime contract: no allocation, no locks, no file or network I/O and no
   unbounded work on the audio thread.
7. `graphify update .` after meaningful source changes, committed with them.
8. A CHANGELOG entry per increment.
9. Project format changes bump `INCDAW_PROJECT_VERSION` and ship a migration —
   an old project must still open.

**Already done for you.** `third_party/clap/clap` is not in git and has been
copied into this worktree; do not delete it and do not commit it.

**Ordering.** The items in your list are ordered. Do them in order unless one
is blocked, and say so if you reorder.
