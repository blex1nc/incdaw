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

## 3. Re-audit — 2026-08-23 · Baseline INCDAW 0.10.0

The list in §1 and §2 was written against 0.9.0. P1–P10 below and sixteen UI
increments have closed most of it. What follows is the gap list re-verified
against the source tree at `5609f0d`, not against the older list — every "have"
was confirmed in code, every "missing" by its absence.

Functional reference re-checked against Image-Line's own pages (the 2026
release page, the features page and the online manual sections named below).

### What INCDAW now has that §2 said it lacked

Time stretch/pitch (WSOLA, offline + clip warp) · slicer with onset detection ·
chorus/flanger/phaser/transient split · sidechain routing · LUFS (BS.1770-4) ·
stereo separation · pre-fader sends · browser with preview and drag/drop ·
AU effect hosting · markers and regions on the timeline · clip split · chord
and note tools · a synthesized piano · **automation touch/latch/write modes**
(`app::AutomationWriteSession::WriteMode`) · per-step levels · theme files.

### Track A — Sound: instruments and effects

| # | Gap | State |
|---|---|---|
| A1 | Wavetable synth | ✗ no wavetable instrument |
| A2 | FM synth | ✗ (the piano's electric voicing has an FM tine; there is no FM *instrument*) |
| A3 | Granular synth | ✗ |
| A4 | Drum machine / pad instrument (FPC, Drumaxx equivalents) | ✗ sampler has zones and layering, nothing pad-oriented |
| A5 | Instrument preset system — save, load, browse, factory presets | ✗ parameters persist per channel; there is no preset object |
| A6 | Multiband compressor | ✗ |
| A7 | De-esser | ✗ |
| A8 | Stereo imaging / mid-side width tool as an insert | ◐ strip-level stereo separation exists; no insert |
| A9 | Waveshaper with a drawable curve | ◐ `incdaw.saturator` is tanh only |
| A10 | Parametric EQ with more than three bands and a draggable curve | ◐ 3 bands; the Tone panel draws a curve but does not let you drag it |
| A11 | Convolution reverb (impulse response loading) | ✗ |
| A12 | Vocoder / formant tools | ✗ |
| A13 | Gross-Beat-style time/volume gating | ✗ |

### Track B — Arrangement: playlist, patterns, automation

| # | Gap | State |
|---|---|---|
| B1 | Clip pan | ⚠ **model field exists, no command and no UI** (`Clip::pan`) |
| B2 | Clip reverse | ⚠ **model field exists, no command and no UI** (`Clip::reversed`) |
| B3 | Clip lock | ⚠ **model field exists, no command and no UI** (`Clip::locked`) |
| B4 | Track folders / groups, collapsible | ⚠ **`TrackType::folder` and `Track::parent` exist, nothing creates or draws one** |
| B5 | Clip grouping (move several as one) | ✗ |
| B6 | Playlist lanes (several clips deep on one track) | ✗ |
| B7 | Crossfades between overlapping clips | ◐ per-clip fades exist; no crossfade verb |
| B8 | Clip consolidation (render a selection to one audio clip) | ✗ |
| B9 | Automation point editing surface — draw, curve tension, scale, copy/paste | ✗ lanes render and record; there is no editor |
| B10 | Automation clip verbs — create from a parameter, duplicate, clear | ◐ commands exist; no workflow around them |
| B11 | Multiple arrangements per project | ✗ one timeline |
| B12 | Performance Mode — trigger clips live from the performance zone, per-track press/motion/sync options, pad and keyboard mapping | ✗ nothing equivalent (engine-deep: realtime clip triggering) |
| B13 | Pattern picker workflow: clone, rename, colour, per-pattern length | ◐ list exists; verbs thin |

### Track C — Integration: hardware, hosting, files

| # | Gap | State | Gate |
|---|---|---|---|
| C1 | MIDI **output** to hardware | **DONE** — `selectOutput`, `engine::MidiOutput` (sender thread, frame→host-time), Settings destination. Channel→external routing needs a `Channel` field (track B) | — |
| C2 | MIDI clock / transport sync (send and receive) | **DONE** — `MidiClockGenerator` off the tempo map, `MidiClockReceiver` with a jitter filter; start/stop/continue/SPP both ways. Applying the received tempo is an open question (no non-undoable path to project tempo) | — |
| C3 | Controller feedback (LEDs, motorised faders) | **DONE (faders)** — `engine::MidiFeedback`, applier-level tap so a lane moves the surface; echo-suppressed. Pad lighting needs a note form on `MidiMapping` (track B) | — |
| C4 | MPE input handling | **DONE (decode)** — `MpeDecoder`, both zones, per-note id, RPN 6/0. Instruments have no per-voice expression input (track `timbre`) | — |
| C5 | Device hot-plug re-enumeration | **DONE** — `platform::DeviceWatcher` (CoreAudio device list + defaults, CoreMIDI setup); MIDI reopens, audio restarts only when the chosen device returns | — |
| C6 | Audio editor: cut / copy / paste between selections | **DONE** — was already implemented and menu-wired; now covered by cross-asset and rate-mismatch tests | — |
| C7 | Audio editor: markers and regions inside the editor | **DONE** — `AudioMarker` stored in the file as RIFF `cue `/`adtl`, coherent under every edit verb, restored by undo; drawn in the editor with add/rename/delete verbs | — |
| C8 | Comping editor over recorded takes | **DONE** — `app::comping::takesOver` + `AssignCompRangeCommand` (split-and-mute, no new model field), `INCDAWCompingView` lanes with per-take waveforms, one undo entry per assignment | — |
| C9 | Pre-record buffer on the input side | **DONE** — a second `AudioLogger` on the input, `RecordingSession::keepPreRoll` lands it as a take ending at the playhead; own switch, off by default | — |
| C10 | Denoise / noise profile | **DONE** — `engine::dsp::Stft` (exact-reconstruction STFT, null-tested), `NoiseProfile` learned per channel, `DenoiseAssetCommand`; Audio → Learn Noise Profile / Denoise… | — |
| C11 | Spectral view and spectral editing | ◐ analyzer draws a spectrum; the editor has no spectral surface | — |
| C12 | VST3 hosting | ✗ format enum only | **§41 — SDK, dual-licensed. Proposal + approval required** |
| C13 | FLAC export/import | ✗ | **§41 — libFLAC. Proposal + approval required** |
| C14 | MP3 export | ✗ | **§41 — LAME (LGPL). AAC/ALAC through AudioToolbox is the zero-dependency alternative and should be proposed alongside** |
| C15 | Out-of-process plugin sandboxing | ✗ | its own program; sequence last |
| C16 | AU **instrument** hosting | ✗ AU effects host fine | — |

### Out of scope, unchanged

Cloud storage and backup (FL Cloud), bundled commercial content (Loop Starter),
AI assistant (Gopher) and ML stem separation, VST2 (D-007).

---

## 4. Closure plan — the 0.9.0 branch (historical)

Ordered by workflow impact × feasibility (no new dependencies, realtime-safe,
every item tested; CLAUDE.md §44 definition of done):

| # | Feature | Status |
|---|---|---|
| P1 | Piano Roll chord toolkit: chord detection (Chord Panel equiv.), Chord Stamp with top-down/bottom-up voice-leading, progression nudge | **DONE** — `app::music`, `ChordCommands`, wired: Option-click stamps, `1`–`8` pick the shape, `V` flips voicing, `C` names the selection, `[`/`]` nudge diatonically |
| P2 | Piano Roll note tools: strum, arpeggiate, legato; note labels | **DONE** — `NoteToolCommands`, wired: `S` strum (+Shift down), `P` arpeggiate (+Shift up-down), `L` legato, F2 rename |
| P3 | Playlist: clip split; timeline markers + regions | **DONE** — `SplitClipCommand` (tick-exact for pattern/automation clips, frame-exact for audio, source offsets advance), `TimelineMarker` model + format v1.5 + `MarkerCommands`; wired: Option-click slices a clip, `M` / Shift+`M` drop a marker / one-bar region at the playhead, ruler draws both |
| P4 | Sidechain routing made functional: compressor external key input, compiled sidechain edges | **DONE** — sidechain edges compile into detector-only inputs on the destination's compressor inserts (PDC-aligned like any edge, key never sums into audio, exact-level tests); mixer strip menu gained "Sidechain Into" |
| P5 | LUFS meter (EBU R128), stereo separation, true pre-fader sends | **DONE** — `incdaw.loudness` insert (BS.1770-4 K-weighting designed per rate, momentary/short-term/gated integrated via histogram, calibrated to the −3.01 LUFS reference at 44.1 and 48 kHz); mid/side stereo separation on every strip (model+format+command+automation key); pre-fader sends tap ahead of the fader through a spliced unity node |
| P6 | Chorus, flanger, phaser; transient/sustain splitter (Transmitter equivalent) | **DONE** — `incdaw.chorus/flanger/phaser` (LFO delay/comb/allpass, bit-exact null at defaults) and `incdaw.transientsplit` (fast/slow envelope race; both/transients/sustain outputs so parallel strips shape each half; exact-unity reassembly) — all in the shared catalogue and the mixer's Add Insert menu |
| P7 | Time-stretch/pitch subsystem (offline, transient-aware) + playlist stretch + editor verbs | **DONE** — `engine::dsp::timeStretch` (WSOLA, shared-channel alignment, transient locking: clicks survive both directions exactly once; pitch via stretch+windowed-sinc; exact identity at 1.0/0); `StretchAssetCommand` (editor Time Stretch…/Pitch Shift…, bit-exact undo of the file), `StretchClipsCommand` + Option-drag at the clip edge (resize-vs-stretch), warped clips render offline at compile time and play through the graph |
| P8 | Slicer: onset detection, slice-to-keys, timing pattern export | **DONE** — `engine::audio::detectOnsets` (energy-leap with refinement, ±6 ms on the test loop, no false onsets in steady tone); `SliceAssetCommand`: one undo lands a sampler channel (one zone per slice, chromatic from C3) plus the pattern replaying the loop's timing through the tempo map; editor menu "Slice to New Channel" |
| P9 | Browser pane: file browsing, search, favourites, preview, drag into project | **DONE** — `app::Browser` (classification, folders-first listings, capped recursive search, favourites, recents, stage-and-rename persistence); `INCDAWBrowserView` (leftmost pane, ⌘B, context menu, drag source); `engine::AuditionPlayer` (preview mixed after the graph, block-counter buffer hand-over, cross-rate repitch, allocation-free); drops land through commands — `LoadSampleCommand` onto a channel, `ImportSampleAsChannelCommand` onto empty rack space, `ImportAudioClipCommand` onto a playlist lane, each one undo with stable ids |
| P10 | AU hosting (system API, no new dependency) | **DONE (effects)** — `platform::AudioUnitHost` (component enumeration that runs no plugin code, stereo float in-place render through AudioUnitRender, parameters, latency, ClassInfo state, generic editor view) behind a header with no CoreAudio type in it; `plugins::HostedPlugin` is now the interface PluginNode, the instance manager, PDC, state files and the editor windows are written against, with `ClapInstance` and `AudioUnitInstance` as its two implementations. Audio Units appear in Add Insert without a scan. Not included: AU instruments, custom Cocoa views, out-of-process AU instantiation |

Out of scope, by recorded product decision (REQUIREMENTS §1.3): cloud storage
and backup, bundled commercial content, AI assistant features, VST2.
