# INCDAW — Performance Strategy

Status: **Phase 0 — targets defined. Nothing measured yet.**

CLAUDE.md §27: performance is a feature, and optimisation without measurement is
forbidden. Every number below is a **target to be verified**, not a claim.

---

## 1. Reference machine

| | |
|---|---|
| CPU | Apple M5, 10 cores (heterogeneous P/E) |
| RAM | 16 GB |
| OS | macOS 26.2 (Darwin 25.2.0), arm64 |
| Disk | ~733 GB free |

The 16 GB ceiling is a real design constraint: it pushes the sampler toward disk
streaming (Phase 14) rather than full preload, and caps how many plugin
instances a project can realistically hold.

---

## 2. Targets

### Audio

At 48 kHz / 128 frames the callback budget is **2.67 ms**.

| Metric | Target |
|---|---|
| Callback, empty project | < 1% of budget |
| Callback, typical project | < 30% of budget |
| Callback, worst observed | < 70% of budget |
| Underruns in a 60-min soak | **0** |
| Round-trip latency @ 128 frames | < 10 ms |

Headroom matters more than the average: a DAW that averages 40% but spikes to
110% glitches audibly, while one that sits at 60% flat does not.

### UI

| Metric | Target |
|---|---|
| Frame time | < 16.6 ms (60 fps) |
| Piano Roll @ 10,000 notes | 60 fps sustained |
| Playlist @ 500 clips | 60 fps sustained |
| Waveform render, 10-min file | < 500 ms (cached thereafter) |

### Project

| Metric | Target |
|---|---|
| Load, medium project | < 2 s |
| Save, medium project | < 1 s |
| Autosave (incremental) | < 200 ms, no audible glitch |
| Startup (with large plugin collection) | < 3 s — must **not** scale with collection size |

---

## 3. Instrumentation

Built in Phase 2 alongside the engine, so no later phase optimises blind:

- Callback duration **histogram** (not just an average) — p50, p95, p99, max
- Per-node processing time in the render graph
- Underrun counter
- Memory: total, per-subsystem, audio-thread allocation count (must be 0)
- UI frame time and dropped frames
- Disk throughput for streaming
- Project load/save timing breakdown

---

## 4. Method

1. **Measure** before touching anything.
2. **Find the actual hot path** — profile, do not guess.
3. **Optimise** the measured bottleneck only.
4. **Re-measure** and record before/after in the commit message.
5. **Guard** with a performance regression test where meaningful.

Optimisations without a recorded measurement are rejected in review, regardless
of how obviously correct they seem.

---

## 5. Known design-level performance decisions

These are architectural rather than micro-optimisations, and are settled now
because they are expensive to change later:

- **Graph compiled off the audio thread.** The audio thread reads an immutable,
  pre-sorted, pre-allocated structure. No topology work in the callback.
- **Metal-rendered editors with viewport culling.** Only visible notes and clips
  are drawn. Required to hit 10,000 notes at 60 fps.
- **Lock-free queues only** across the realtime boundary.
- **Preallocated buffers**, sized at graph compile.
- **Disk streaming** for large samples and audio clips.
- **Lazy loading** of media and plugin state from the project package.
- **Persistent plugin registry** so startup never rescans.
- **Denormals disabled** (FTZ/DAZ) at callback entry — untreated denormals cause
  large, mysterious CPU spikes on decaying tails.
- **`os_workgroup` membership** for all realtime threads so the scheduler keeps
  them on performance cores (D-004).

---

## 6. Profiling tooling

`os_signpost` instrumentation is added in Phase 2 and is usable from the command
line. **Instruments.app requires full Xcode, which is not installed** — only
Command Line Tools. If deep GPU or system-level profiling proves necessary in
Phase 18, installing Xcode becomes a dependency decision at that point. It is
not required before then, and this document does not assume it.

---

## 7. Phase 18 — measured baseline and optimisations

Measured with `incdaw-bench` (tools/bench, Release, Apple M5, 48 kHz). The
tool is committed; every number below is reproducible with
`./build/incdaw-bench`. Medians over 20 runs for compile, single runs for
the throughput lines (variance is small; the block-cost lines average 4,000
blocks).

### Baseline, 2026-08-16

| Measurement | Result | Budget/context |
|---|---|---|
| Graph rebuild, 16 sampler channels + master chain | 0.049 ms | per edit; imperceptible |
| Graph rebuild, 64 sampler channels + master chain | 0.158 ms | per edit; imperceptible |
| Offline render, 64 channels, 4 s song | 63.4 ms | 63× realtime |
| Every builtin effect, 512-frame stereo block | 0.2–8.6 ns/frame | all < 0.05 % of block budget |
| Resample 10 s stereo 48 → 44.1 kHz | **547.9 ms** | 18.3× realtime |
| Piano Roll culling, 10,000 notes (Phase 6 record) | 0.012 ms/frame | 16.6 ms budget |

Reading of the baseline: rebuild latency and the realtime path have head
room everywhere — no optimisation is justified there (CLAUDE.md §27). Two
findings demanded action:

1. **The benchmark itself lied about the EQ.** Parameters were set to
   range midpoints; the EQ's mid gain midpoint is 0 dB, which the effect
   skips as identity — so 0.2 ns/frame was the cost of a bypass. Fixed by
   setting parameters to 75 % of range. Honest EQ cost: **15.6 ns/frame**
   (0.07 % of budget) — fine, no optimisation needed.
2. **The resampler evaluated `sinc` and a 4-term window per tap per
   frame** — two transcendental calls × 64 taps × every output sample.

### Optimisation: table-driven resampler kernel, 2026-08-16

The windowed-sinc kernel is now precomputed into a 512-phase × 64-tap
table per `resample` call, with linear interpolation between adjacent
phase rows. Phase-interpolation error sits far below the window's own
−92 dB sidelobes; the quality regression test (RMS error of a converted
1 kHz tone < −60 dB, RenderTests.cpp) passes unchanged.

| | Before | After | Change |
|---|---|---|---|
| Resample 10 s stereo 48 → 44.1 kHz | 547.9 ms (18.3×) | **18.6 ms (537.2×)** | **29× faster** |

Bench numbers after the change (unchanged elsewhere, within noise):
compile 64ch 0.197 ms, render 69.9× realtime, effects 0.5–15.6 ns/frame.

### Not optimised, deliberately

- Rebuild latency (0.2 ms at 64 channels): three orders of magnitude
  inside "instant"; any work here is speculative.
- Builtin effects (< 0.1 % of budget each): the mix bus would need
  hundreds of inserts before this shows on a meter.
- The Sampler's per-frame LFO/filter path: gated off unless depths are
  set; at 64 held voices the render bench shows 70× realtime headroom.
- UI frame profiling beyond the Phase 6 culling record needs
  Instruments.app (full Xcode) — a dependency decision recorded in §6,
  still not forced.

---

## Instrument cost — the piano's voice budget (2026-08-23)

The piano is a modal model — up to 24 rotating partials per voice — so unlike
the reference synth its cost scales with how much of the keyboard is sounding.
Measured with `incdaw-bench` (Release, arm64), 512-frame stereo blocks at
48 kHz, sustain pedal down so nothing retires, over ~4 seconds of audio.

| Workload | Share of the block budget |
|---|---|
| Piano, Grand, 1 voice | 0.05 % |
| Piano, Grand, 16 voices | 1.08 % |
| Piano, Grand, 64 voices (full polyphony) | **4.20 %** |
| Piano, Electric, 64 voices | 0.97 % |
| Reference synth, 32 voices | 0.82 % |

A whole keyboard held down with the pedal costs about a twentieth of one
callback, which is the number that mattered: the rotation-based oscillator
was chosen over `std::sin` per partial per sample precisely so that this
would be affordable, and it is.

The measurement window is deliberately ~4 s. An earlier run measured 21 s and
reported the electric at 0.23 %, which was the cost of silence — the voices
had decayed away before the benchmark ended.

### Not optimised, deliberately

- The piano at anything under full polyphony. Four percent at 64 voices leaves
  no case for tuning a path that costs one percent in ordinary playing.
