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
