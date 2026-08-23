# Agent 2 — Effects Archive (+31 → 48)

Branch: `feat/plugin-effects`. Read `00-CONTRACTS.md` first. Start only after
Agent 1's Wave 0 is on `main`.

Existing 17 (do not rewrite): utility, filter, eq, tone, saturator,
compressor, limiter, limiterla, gate, delay, reverb, analyzer, loudness,
chorus, flanger, phaser, transientsplit.

You own: `engine/dsp/effects/<Family>Effects.{h,cpp}` for **new** families,
`app/devices/<Family>Panels.cpp`, `tests/unit/<Family>EffectTests.cpp`.
You never touch `main.mm`, `DeviceUiSpec.h` or `ProjectGraphCompiler.cpp`.

Reuse before writing: `engine/dsp/Fft`, `Resampler`, `TimeStretch`,
`DelayLineNode`, `OnsetDetection`, and the biquad/envelope helpers already in
`ToneEffects`/`DynamicsEffects`. Duplicated DSP is a review failure (§34).

---

## Wave E1 — Dynamics and mastering (5)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.mbcomp` | Spectra Comp | 3–4 band Linkwitz-Riley crossover, per-band compressor reusing `CompressorEffect`'s detector; sum must reconstruct unity when all bands bypass | crossover `drawable-curve` + per-band GR `meter` + knob rows |
| `incdaw.deesser` | Sibilance | split-band and wideband detector modes, sidechain-filtered gain reduction | `spectrum` with the detector band overlaid, threshold/range knobs |
| `incdaw.maximizer` | Ceiling | soft-knee multiband gain + true-peak-aware inter-sample ceiling; reuse the lookahead of `LookaheadLimiterEffect` | transfer `drawable-curve` + live LUFS readout from `incdaw.loudness`'s BS.1770 code |
| `incdaw.clipper` | Hard Edge | selectable soft/hard/quintic clip shapes, oversampled 4× to control aliasing | transfer `drawable-curve`, drive/ceiling knobs |
| `incdaw.envfollow` | Follower | envelope detector whose output is a modulation source (Agent 1 Wave 4 plumbing) | input `meter` + response `envelope` |

## Wave E2 — Tone and spectral (8)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.eqp` | Contour EQ | 8 bands, each bell/shelf/pass/notch, plus dynamic (threshold-driven) bands; coefficients designed per sample rate | draggable `drawable-curve` over a live `spectrum` — the flagship panel |
| `incdaw.graphiceq` | Grid EQ | 10-band constant-Q graphic | `fader-wall` |
| `incdaw.tilt` | Tilt | tilt shelf pair + low/high emphasis | 2 knobs + `drawable-curve` |
| `incdaw.formant` | Formant | 3–5 formant resonator bank, vowel morphing | vowel `xy-pad` |
| `incdaw.fsplit` | Splitter | multiband split feeding parallel mixer strips (same pattern `transientsplit` uses for its multiple outputs) | crossover bars |
| `incdaw.fshift` | Shifter | Hilbert transform single-sideband frequency shift (not pitch shift) | shift dial + `spectrum` |
| `incdaw.vocoder` | Vocoder | N-band (16/24/32) analysis of a sidechain modulator onto the insert's carrier; uses the sidechain key input already compiled for the compressor | band `matrix`, band-count `combo`, formant/attack knobs |
| `incdaw.shaper` | Curve | drawable transfer function, oversampled, with symmetry and bias | drawable transfer curve (closes gap A9) |

## Wave E3 — Distortion and space (7)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.drive` | Drive | multi-algorithm: tube asymmetric, wavefold, diode, fuzz; pre/post tone stack | algorithm `combo` + transfer curve |
| `incdaw.bitcrush` | Crush | bit depth quantization + sample-rate decimation with anti-alias option | staircase `waveform` |
| `incdaw.convolver` | Convolver | **partitioned uniform FFT convolution** over `engine/dsp/Fft`; IR loaded off the audio thread through `AssetResolver`; reports its latency for PDC exactly as the lookahead limiter does | IR `waveform`, envelope, stretch, wet/dry (closes A11) |
| `incdaw.reverb2` | Luxe | feed-forward **and** feedback modes, modulated delay network, decay-vs-frequency damping | decay-vs-frequency `drawable-curve` |
| `incdaw.mbdelay` | Band Delay | per-band delay bank over the crossover of `fsplit` | band `grid` of time/feedback/pan |
| `incdaw.tapedelay` | Tape | wow/flutter modulation, filtered saturating feedback, tempo sync | tape transport graphic (custom-view candidate) |
| `incdaw.grainverb` | Grain Delay | granular / reversed delay buffer, pitch per grain via `Resampler` | grain cloud (custom-view candidate) |

## Wave E4 — Modulation, stereo, performance (11)

| uid | Name | DSP core | Panel |
|---|---|---|---|
| `incdaw.vchorus` | Vintage Chorus | BBD-style companded delay, 3 stages | stage lamps + rate/depth |
| `incdaw.ensemble` | Ensemble | 6-voice detuned chorus with decorrelated LFOs | voice spread graphic |
| `incdaw.rotary` | Rotary | horn/drum doppler + crossover + speed ramp | speed rocker + `xy-pad` |
| `incdaw.ringmod` | Ring | ring modulation, tremolo and auto-pan under one LFO section | LFO shape + `xy-pad` |
| `incdaw.imager` | Imager | per-band mid/side width; reuse the M/S maths of the mixer strip's stereo separation | `goniometer` + correlation `meter` + band widths (closes A8) |
| `incdaw.spreader` | Spread | Haas and allpass decorrelation, mono-safe | width arc |
| `incdaw.scope` | Scope | oscilloscope / goniometer / vectorscope from a lock-free snapshot | `scope` |
| `incdaw.tuner` | Tuner | YIN pitch detection, cents deviation | strobe dial |
| `incdaw.pitch` | Pitch | realtime shift with formant preservation over `TimeStretch`'s WSOLA path | interval wheel |
| `incdaw.grossgate` | Motion | time-position and volume gating curves with a slot bank, tempo-locked; the time curve reads from a look-back buffer (closes A13) | dual `drawable-curve` + slot bank |
| `incdaw.formula` | Formula | expression-evaluated modulation source; **expression is compiled off the audio thread into a flat opcode buffer** — no parsing, no allocation in `process` | text editor + result `scope` |

---

## Tests you must write

Per device: null/unity at defaults · analytic DSP check (e.g. the crossover
sums flat, the Hilbert pair is 90° apart, the convolver against a direct
time-domain convolution of a short IR) · state round-trip · parameter registry
presence · NaN/Inf/denormal input survival · no allocation in `process`.

Latency-reporting devices (`convolver`, `maximizer`, `mbcomp` if lookahead)
need a PDC test in the style of `PluginLatencyTests`.

## Definition of done

`00-CONTRACTS.md` §6, all ten points, per device. Ship a wave only when every
device in it is complete — a half-done wave does not merge.
