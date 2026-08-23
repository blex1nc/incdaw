# Agent 3 — Instruments Archive (+17 → 20)

Branch: `feat/plugin-instruments`. Read `00-CONTRACTS.md` first. Start only
after Agent 1's Wave 0 is on `main`.

Existing 3 (do not rewrite): `SimpleSynth`, `Sampler` (key/velocity zones,
layering, disk streaming), `PianoInstrument`.

You own: `engine/instrument/<Name>.{h,cpp}`,
`project/instruments/<Family>Factory.cpp`, `app/devices/<Family>Panels.cpp`,
`tests/unit/<Name>Tests.cpp`. You never touch `main.mm`, `DeviceUiSpec.h`,
`BuiltinEffects.cpp` or `ProjectGraphCompiler.cpp`.

**The base class does the hard part for you.** `Instrument::processBlock` is
`final` and already splits the block so every MIDI message lands on its exact
frame. Implement `handleMessage` and `renderRange` only; never re-implement
event timing (`Instrument.h` says why).

`prepare()` may allocate. `renderRange()` may not — voices, delay lines and
tables are sized in `prepare`. `renderRange` **adds** to the output buffer, it
does not overwrite it.

---

## Wave I1 — Synthesis cores (4)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.wavetable` | Morph | 2 wavetable oscillators, position morph, band-limited mipmapped tables built by the Wave 4 asset tool; sub osc, noise, filter, 2 envelopes, 2 LFOs | 3D wavetable display (**custom view**) + envelope/filter sections (closes A1) |
| `incdaw.fm` | Matrix FM | 6 operators, free modulation matrix, per-operator ratio/fixed/level/envelope, feedback | operator `matrix` + per-op `envelope` (closes A2) |
| `incdaw.va` | Polyphon | 3 band-limited analog-style oscillators, unison with detune and stereo spread, ladder + SVF filter modes | classic knob panel |
| `incdaw.additive` | Spectra | partial bank with drawable harmonic amplitude and detune profiles, per-partial decay | harmonic `fader-wall` + drawable profile |

## Wave I2 — Rhythm (4)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.drumpad` | Pad Kit | 16 pads, each a multisample layer stack with velocity layering, choke groups, per-pad filter/envelope/pan; reuses the `Sampler` zone engine rather than a second sampler | `pad-grid` + per-pad edit strip (closes A4) |
| `incdaw.drumsynth` | Analog Kit | fully synthesized kick/snare/hat/clap/tom parts — no samples | part `tab`s |
| `incdaw.kick` | Kick | pitch envelope + amplitude envelope + click/body/saturation stages | dual `drawable-curve` |
| `incdaw.membrane` | Membrane | modal synthesis: bank of tuned resonators excited by an impulse, per-mode decay | modal `grid` |

## Wave I3 — Sample, character, utility (9)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.granular` | Cloud | grain scheduler over a cached buffer, grain size/density/spray/pitch/window, stereo scatter; grains pre-allocated in `prepare` | `waveform` + grain cloud (**custom view**) (closes A3) |
| `incdaw.slicer` | Slice | playable slice instrument over `engine::audio::detectOnsets`; each slice on a key, per-slice pitch/reverse/envelope | `waveform` with draggable slice markers |
| `incdaw.rompler` | Flare | preset-driven multisample instrument with 8 macros — the archive's browsable flagship; leans entirely on Agent 1's preset system | preset browser + 8 macro knobs (closes the A5 workflow) |
| `incdaw.sfz` | SFZ Player | SFZ subset parser (region, key/vel range, loop, offset, volume, pan, ampeg) mapping onto the `Sampler` zone engine | `zone-map` |
| `incdaw.layer` | Layer | hosts child instruments with key/velocity splits and per-layer transpose/level; a channel that renders other channels | layer lanes + `keyboard` |
| `incdaw.pluck` | String | extended Karplus-Strong: excitation shaping, damping, stiffness/dispersion allpass, sympathetic coupling | string diagram |
| `incdaw.bass` | Slide Bass | monophonic saw/square with slide (portamento), accent, resonant ladder envelope | 2-row knob panel + accent strip |
| `incdaw.organ` | Drawbar | 9 tonewheel drawbars, percussion, key click, vibrato/chorus scanner; pairs with `incdaw.rotary` | drawbar `fader-wall` |
| `incdaw.noise` | Texture | filtered/spectral noise generator with morphing spectrum for pads and atmospheres | `spectrum` morph |

Custom-view budget used: 2 of the 8 allowed (wavetable display, grain cloud).
Ask Agent 1 before spending a third.

---

## Asset-bearing instruments

`wavetable`, `drumpad`, `rompler`, `sfz`, `granular` and `slicer` need decoded
assets. They construct through `InstrumentBuildContext::assets`
(`00-CONTRACTS.md` §3.2) — never by reading a file in `prepare` and never on
the audio thread. All shipped wavetables and drum samples come from Agent 1's
generator tool in `tools/`, are originally produced, and carry no third-party
library content (§20, §43).

## Voice management

Every polyphonic instrument needs a stated voice cap, a documented steal
policy (oldest / quietest / same-note) and a release tail that finishes rather
than clicking. Test that exceeding the cap steals instead of allocating —
`activeVoiceCount()` exists for exactly this.

## Tests you must write

Per instrument: silence with no MIDI · deterministic, reproducible output for
a fixed note (golden or analytic) · note-on/note-off timing lands on the exact
frame (extend `InstrumentTests`' timing probe pattern) · `allNotesOff()`
silences immediately · voice steal at the cap · no allocation in `renderRange`
· NaN/Inf-free output across the parameter range · state round-trip ·
parameter registry presence.

`incdaw.layer` additionally needs a recursion guard test — a layer must not be
able to host itself.

## Definition of done

`00-CONTRACTS.md` §6, all ten points, per device. Ship a wave only when every
device in it is complete.
