# INCDAW — Requirements

Status: **Phase 0 — scope defined. Nothing implemented.**

This document records *what INCDAW must eventually do* and *what the functional
reference actually does*. It is the source for phase planning in
docs/ROADMAP.md.

---

## 1. Functional reference: FL Studio 2026

FL Studio is a **functional reference only**. INCDAW reproduces capabilities and
workflows, never implementation, source, assets, presets, branding, or visual
identity (CLAUDE.md §43).

Findings below are drawn from Image-Line's **official** release page and online
manual, gathered during Phase 0 discovery — not from memory and not from
inspecting the FL Studio installation present on this machine.

### 1.1 Findings that changed INCDAW's architecture

These are the items that had real design consequences, not just feature-list
entries:

| FL Studio 2026 fact (official) | Consequence for INCDAW |
|---|---|
| macOS now uses the **Audio Workgroups API** for audio thread scheduling, reducing underruns | Independent validation of D-004. A shipping professional DAW made the same call on the same hardware class. |
| macOS audio settings gained **separate input and output device selectors** | The device layer must not assume a single duplex device (docs/AUDIO_ENGINE.md §2). |
| **MIDI recording** improved: reduced jitter, better tempo-change handling | Timestamped MIDI input and tempo-map-aware conversion must be designed in from Phase 5, not retrofitted. |
| Piano Roll notes can be **renamed / labelled** | MIDI events need an extensible per-note property slot, not a fixed struct. Affects the core event type. |
| **Audio Logger** retains the last 60 s of master output | A ring buffer at the master node — cheap if designed in now, invasive later. Specified in docs/AUDIO_ENGINE.md §8. |
| **Transmitter** splits a signal into transient and sustain streams, each routable to its own mixer track | The mixer graph must support multi-output plugin nodes; a linear insert chain would not suffice. |
| Audio clips gained **gain**, **pan** and **normalization** (individual or group-relative) | Clip-level gain/pan are clip *properties* applied pre-mixer, and must round-trip through the project format. |
| Disk recording offers **16 vs 32-bit** | Bit depth is a per-recording parameter, not a global setting. |
| The assistant can organise tracks, route mixer channels, set levels, adjust plugin parameters and generate Piano Roll content | Only possible because FL exposes a scriptable command surface. Strong support for the command-registry-from-day-one decision (docs/ARCHITECTURE.md §6). |
| Plugin Manager: faster scanning | Scanning is a known pain point at scale; INCDAW caches aggressively and never rescans at startup. |
| Project backups embed **save dates in filenames** | Adopted for `history/` (docs/PROJECT_FORMAT.md §5). |

### 1.2 Supported plugin formats (official)

- **macOS:** 64-bit VST 1/2, VST3, 64-bit Audio Units, CLAP
- **Windows:** 32/64-bit VST 1/2, VST3, CLAP, Image-Line native format

INCDAW targets **CLAP, AU and VST3**. VST2 is excluded — Steinberg ended VST2
SDK licensing in 2018 and there is no lawful route for a new closed-source
product (D-007). This is the single deliberate divergence from the reference.

### 1.3 Other FL Studio 2026 features, recorded for completeness

Piano Roll: Chord Panel with real-time note/chord detection; Chord Stamp with
top-down and bottom-up voice-leading; Chord Progression tool with nudge; MIDI
score import (`.mid`, `.fsc`).
Playlist: explicit resize-vs-stretch cursor distinction; Performance Mode marker
setup for clip-column triggering.
Mixer: Alt+click resets I/O selectors; empty insert slots open the plugin menu
directly.
Plugins: FLEX rebuilt (~50% lower CPU); Luxeverb pitch feed-forward mode; FPC
multi-sample layering; DirectWave multi-zone selection; Fruity Slicer 2
redesigned.
Project/Cloud: FL Cloud project backup (500 MB free, up to 1 TB), encrypted;
Account tab in settings.
Scripting: MIDI scripting gained pattern management, swing control and undo/redo
flags; MCU support moved to a dedicated script.

**Out of scope for INCDAW:** cloud storage/backup services, bundled commercial
content libraries, and any AI assistant feature. These are product decisions,
not architectural gaps.

---

## 2. INCDAW functional scope

The long-term target, from CLAUDE.md §1–§34. Phase assignments are in
docs/ROADMAP.md. Nothing here is implemented.

### Transport
Play, pause, stop, record, loop, pattern and song modes, metronome, count-in,
tempo, time signature, playhead, pre/post-roll, punch in/out, start/stop
markers, snap, quantization, CPU and audio indicators.

### Project system
New, save, save as, open, recent, recovery, autosave, backup, metadata,
versioning, migration, templates, packaging, sample dependency management,
missing-file detection, relinking, archiving. **Versioned from day one.**

### Audio engine
Realtime multichannel processing, sample-accurate scheduling, realtime-safe
processing, deterministic transport, low latency, device abstraction, I/O
routing, sample rate and buffer size control, latency compensation, PDC,
offline and realtime rendering, bounce, freeze, resampling, interpolation,
oversampling, clipping detection, metering.

### MIDI engine
Input, output, recording, playback, editing, velocity, note length/start/end,
pitch, modulation, pitch bend, aftertouch (incl. polyphonic), CC, program
change, MIDI clock and sync, routing, thru, quantization, humanization,
mapping. MPE-ready.

### Piano Roll
Note create/delete/move/resize/duplicate, multi/box/lasso selection, velocity
editing, channel colouring, quantize, scale snapping, ghost notes and patterns,
chord tools and generation, strum, arpeggiation, legato, articulation, slide,
portamento, note probability, repeat, grouping, CC/pitch-bend/modulation lanes,
grid customisation, zoom, scroll, undo. Must remain usable at very large note
counts.

### Channel / instrument system
Channels representing instruments, samplers, audio sources, MIDI sources,
generators, plugins, or external MIDI devices. Mute, solo, volume, pan, routing,
instrument assignment, MIDI I/O, state, presets, automation, colour, naming,
grouping.

### Step sequencer
Step and pattern programming, velocity, note repeat, probability, per-step
parameters, swing, accents, pattern length, multiple time divisions, polymetric
lengths, MIDI generation.

### Pattern system
Reusable, independently editable patterns containing MIDI, automation,
controller events and generated sequences, placeable in the arrangement.

### Playlist / arrangement
Audio, MIDI, pattern and automation clips; instrument and audio tracks; markers
and regions; split, resize, duplicate, stretch, reverse, fade, crossfade, clip
gain, clip pitch, time-stretch, consolidate, group, lock, colour; track folders
and groups, lanes, overlapping clips, free placement. **No assumption of a 1:1
track-to-mixer relationship.**

### Automation
Points, curves, ramps, step transitions, tension, smoothing; clip-based and
parameter automation; MIDI automation; recording and recording modes; editing,
linking, parameter mapping, scaling, copy/paste. **One generic subsystem — never
per-plugin automation code.**

### Mixer
Scalable track count, inserts, sends, returns, buses, subgroups, master, routing
matrix, sidechain routing, pre/post routing, volume, pan, mute, solo, polarity,
stereo separation, gain, peak and RMS metering, LUFS-ready architecture, plugin
chains and bypass, plugin latency and PDC, automation, naming, colouring.
**Graph-like routing, no hardcoded linear chains.**

### Plugin host
Scanning, discovery, categorisation, load/unload, state, presets, automation,
parameter discovery and mapping, editor embedding, offline and realtime
processing, latency reporting, crash isolation, sandboxing, validation,
blacklist, rescan.

### Instruments and effects
An extensible instrument framework (sampler, subtractive, wavetable, FM,
granular, drum machine, multisampler) and an effect framework (EQ, compressor,
limiter, gate, expander, saturation, distortion, waveshaper, reverb, delay,
chorus, flanger, phaser, stereo tools, filters, transient shaping, de-esser,
utility gain, analyzers) sharing one DSP interface. **Build the API before the
instruments.**

### Audio editor
Waveform view, zoom, selection, trim, cut/copy/paste, fade, crossfade,
normalize, gain, reverse, silence, denoise with noise profile, spectral-editing
architecture, resample, pitch shift, time stretch, markers, regions, loop
selection, analysis, recording.

### Time stretching and pitch
Pluggable architecture: realtime preview, offline high-quality mode, pitch
shifting, time stretching, tempo sync, transient preservation, formant-aware
architecture, algorithm selection. **A low-quality placeholder does not count as
this feature.**

### Recording
Microphone, line and instrument inputs; multiple simultaneous inputs; monitoring;
latency compensation; recording modes; loop recording; take management;
comping-ready architecture; punch recording; pre-record buffer; continuous
background capture; recording into playlist and editor.

### Sampler
Sample loading, waveform, start/end, loop points, crossfade loop, root note, key
range, velocity mapping, pitch, time stretch, reverse, envelopes, filters, LFO,
modulation, layering, multisampling.

### Browser and content
Samples, instruments, presets, plugins, projects; folders, favourites, tags,
search, preview, drag and drop, metadata, recent items, user libraries.
**No bundled copyrighted commercial content** — original, demo or public-domain
assets only.

### MIDI controllers and linking
Keyboard input, controllers, knobs, faders, pads, transport controls, mapping,
learn mode, custom mappings, hardware feedback, MIDI clock, synchronisation. A
generic parameter mapping system able to target mixer, instrument, plugin,
transport, automation and macro parameters.

### Undo / redo
Command-based, across project edits, MIDI, audio, automation, mixer, routing,
plugin parameters and appropriate UI state. **No ad-hoc per-feature undo.**

### UI
Modular workspaces: toolbar, playlist, piano roll, channel rack, mixer, browser,
plugin windows, audio editor, project/MIDI/audio settings. Dockable and
resizable panels, scalable UI, keyboard shortcuts, context menus, customisable
layout, workspace persistence, dark professional interface,
accessibility-ready. **INCDAW's own visual identity — not FL Studio's.**

### File formats
Audio: WAV, AIFF, FLAC (MP3 where licensing and platform allow).
MIDI: Standard MIDI File import/export.
Project: native versioned `.incdaw` with migration.
Export: WAV, FLAC, stems, selected regions, master, mixer tracks.

---

## 3. Non-functional requirements

| Requirement | Source | Verified by |
|---|---|---|
| Zero allocations/locks on the audio thread | CLAUDE.md §3 | Realtime guard, from Phase 2 |
| Sample-accurate event timing | §29 | Transport tests, Phase 3 |
| Offline render == realtime render | §31 | Equivalence test, Phase 17 |
| Project format versioned from v1.0 | §2 | Fixture tests, Phase 4 |
| Plugin crash does not kill INCDAW | §12 | Misbehaviour matrix, Phase 13 |
| UI decoupled from DSP | §34 | Layering test, Phase 1 |
| Every action is a command | §26 | Layering test + review |
| Regression test for every serious bug | §28 | Review policy |
| Permissive-licensed dependencies only | D-008 | Decision log review |
