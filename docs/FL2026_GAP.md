# INCDAW — FL Studio 2026 Gap Analysis

Date: 2026-08-16 · Baseline: INCDAW 0.9.0 (phases 0–20 complete)

Functional reference: FL Studio 2026, from Image-Line's official release page and
online manual (see docs/REQUIREMENTS.md §1 for the Phase 0 findings; this
document adds the post-0.9.0 gap list and tracks closure). FL Studio remains a
functional reference only — capabilities and workflows, never implementation,
assets or branding (CLAUDE.md §43).

Status legend: ✗ missing · ◐ partial · ✓ covered · ⊘ out of scope by decision

---

## 1. FL Studio 2026 headline features vs INCDAW 0.9.0

| FL Studio 2026 | INCDAW 0.9.0 | Status |
|---|---|---|
| Chord Panel — real-time note/chord detection in the Piano Roll | no chord analysis anywhere | ✗ |
| Chord Stamp — insert chords, top-down / bottom-up voice-leading | no chord tools | ✗ |
| Chord Progression tool with chord nudge | no chord tools | ✗ |
| Note labeling (rename notes) | per-note property slot exists (Phase 5 design); no tool | ◐ |
| Transmitter — transient/sustain split, each independently shapeable | nothing equivalent | ✗ |
| Fruity Slicer 2 — slice audio, replay slices from keys, rearrange | no slicing anywhere | ✗ |
| FLEX rebuilt — preset-driven instrument with browser | instruments: SimpleSynth, Sampler only | ✗ |
| Luxeverb feed-forward mode | builtin reverb exists; no comparable mode | ◐ |
| FPC multi-sample layering | Sampler has key/velocity zones + layering; no pad-oriented drum instrument | ◐ |
| DirectWave multi-zone selection | multisampling exists; multi-zone edit is an editor nicety | ◐ |
| Playlist resize-vs-stretch cursor distinction | **no time-stretch subsystem at all** | ✗ |
| Performance Mode — marker-driven clip triggering | no markers, no performance mode | ✗ |
| Audio clip gain / normalize (single or group-relative) | clip gain + normalize since Phase 9b | ✓ |
| Audio Logger (last 60 s of master) | Phase 12 part 8 | ✓ |
| MIDI recording: low jitter, tempo-change-aware | designed in from Phase 5 | ✓ |
| Separate input/output device selectors | device layer never assumed duplex | ✓ |
| Audio Workgroups scheduling | D-004, Phase 2 | ✓ |
| Faster plugin scanning | cached registry, never rescans at startup | ✓ |
| Dated project backups | `history/` with save dates | ✓ |
| MIDI score import (.mid) | SMF import/export, Phase 17 (`.fsc` is proprietary) | ✓ |
| 16 vs 32-bit disk recording | recorder bit depth — verify per-recording option | ◐ |
| Plugin formats: VST2, VST3, AU, CLAP | CLAP only; AU/VST3 planned; VST2 excluded (D-007) | ◐ |
| Mixer niceties: Alt+click I/O reset, insert slot opens plugin menu | not present | ✗ |
| FL Cloud backup, Loop Starter content, subscriptions | product decision: no cloud services | ⊘ |
| Gopher agentic assistant | product decision: no AI assistant; the command registry is the scriptable surface it would need | ⊘ |
| Remix a Song (tempo detect + ML stem separation) | ML stem separation falls under the no-AI product decision | ⊘ |

## 2. Baseline FL Studio capabilities INCDAW still lacks

Not new in 2026, but part of the program INCDAW measures against:

- **Time stretching / pitch shifting** — anywhere: playlist clips, audio editor,
  sampler. The single largest functional gap (REQUIREMENTS §2 "Time stretching
  and pitch"; a low-quality placeholder does not count).
- **Playlist**: clip split, crossfades, clip pan, clip reverse; markers and
  regions; track folders; lanes; clip grouping and locking.
- **Mixer**: sidechain edges serialize but compile to nothing; pre-fader sends
  behave as post-fader; no stereo separation control; no LUFS loudness meter.
- **Effects**: no chorus, flanger, phaser, transient shaper, de-esser,
  stereo imaging tools. (EQ, compressor, limiter, gate, expander, saturation,
  delay, reverb, filters, utility, analyzer exist.)
- **Instruments**: no wavetable, FM, granular, or pad-oriented drum machine.
- **Browser**: no browser at all — samples, presets, plugins, projects, search,
  favourites, preview, drag and drop.
- **Automation**: no touch/latch recording modes, no dedicated point-editing
  surface, no copy/paste or scaling UI.
- **Audio editor**: no markers/regions, no cut/copy/paste between selections,
  no denoise, no spectral view.
- **Recording**: takes stack but there is no comping editor; no input-side
  pre-record buffer.
- **File formats**: no FLAC, no MP3 (licensing/deps to evaluate).
- **MIDI**: MPE-ready representation but no MPE handling; no MIDI scripting
  surface beyond the command registry.
- **Plugin hosting**: no AU, no VST3, no sandboxed in-process crash recovery.

## 3. Closure plan — this branch

Ordered by workflow impact × feasibility (no new dependencies, realtime-safe,
every item tested; CLAUDE.md §44 definition of done):

| # | Feature | Status |
|---|---|---|
| P1 | Piano Roll chord toolkit: chord detection (Chord Panel equiv.), Chord Stamp with top-down/bottom-up voice-leading, progression nudge | **DONE** — `app::music`, `ChordCommands`, wired: Option-click stamps, `1`–`8` pick the shape, `V` flips voicing, `C` names the selection, `[`/`]` nudge diatonically |
| P2 | Piano Roll note tools: strum, arpeggiate, legato; note labels | **DONE** — `NoteToolCommands`, wired: `S` strum (+Shift down), `P` arpeggiate (+Shift up-down), `L` legato, F2 rename |
| P3 | Playlist: clip split; timeline markers + regions | **DONE** — `SplitClipCommand` (tick-exact for pattern/automation clips, frame-exact for audio, source offsets advance), `TimelineMarker` model + format v1.5 + `MarkerCommands`; wired: Option-click slices a clip, `M` / Shift+`M` drop a marker / one-bar region at the playhead, ruler draws both |
| P4 | Sidechain routing made functional: compressor external key input, compiled sidechain edges | **DONE** — sidechain edges compile into detector-only inputs on the destination's compressor inserts (PDC-aligned like any edge, key never sums into audio, exact-level tests); mixer strip menu gained "Sidechain Into" |
| P5 | LUFS meter (EBU R128), stereo separation, true pre-fader sends | pending |
| P6 | Chorus, flanger, phaser; transient/sustain splitter (Transmitter equivalent) | pending |
| P7 | Time-stretch/pitch subsystem (offline, transient-aware) + playlist stretch + editor verbs | pending |
| P8 | Slicer: onset detection, slice-to-keys, timing pattern export | pending |
| P9 | Browser pane: file browsing, search, favourites, preview, drag into project | pending |
| P10 | AU hosting (system API, no new dependency) | pending |

Out of scope, by recorded product decision (REQUIREMENTS §1.3): cloud storage
and backup, bundled commercial content, AI assistant features, VST2.
