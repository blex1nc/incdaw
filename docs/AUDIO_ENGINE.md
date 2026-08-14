# INCDAW — Audio Engine

Status: **Phase 0 — design only. Not implemented.**
Target: Phase 2 (device layer + realtime thread) and Phase 3 (transport).

---

## 1. The prime directive

The audio thread never allocates, never locks, never performs I/O, never waits
on the UI, and never performs an operation whose duration it cannot bound.

Everything else in this document is a consequence of that sentence.

---

## 2. Device layer

Backed by CoreAudio HAL directly (docs/DECISIONS.md D-003), behind an
INCDAW-owned interface in `platform/`.

```
AudioDevice
 ├── enumerate()            available input and output devices
 ├── open(config)           sample rate, buffer size, channel map
 ├── start() / stop()
 ├── latency()              input + output + safety offset, in frames
 ├── workgroup()            os_workgroup_t from the device
 └── onDeviceChange()       hot-plug, default-device change, format change
```

**Separate input and output device selection is required.** FL Studio 2026's
release notes record adding separate macOS input/output selectors; the same need
applies here. The device layer must not assume one duplex device, and must
handle the aggregate/mismatched-clock case explicitly rather than by accident.

Properties that must be read and honoured, not guessed:

| Property | Why |
|---|---|
| `kAudioDevicePropertyBufferFrameSize` | Callback size; drives all budgets |
| `kAudioDevicePropertySafetyOffset` | Real latency is larger than buffer size |
| `kAudioDevicePropertyLatency` (in + out + stream) | Required for correct PDC and recording alignment |
| `kAudioDevicePropertyNominalSampleRate` | Rate changes must be handled, not crashed on |
| `kAudioDevicePropertyIOThreadOSWorkgroup` | See §3 |

Reported latency is what makes recorded audio land in the right place. Getting
it wrong is silent and only discovered much later, so Phase 12 includes a
measured loopback alignment test rather than trusting the arithmetic.

---

## 3. Realtime thread scheduling

Verified available in the macOS 26.5 SDK during Phase 0 discovery:
`os/workgroup.h`, `os/workgroup_interval.h`, `os/workgroup_parallel.h`, and
`AudioToolbox/AudioWorkInterval.h`.

- The audio callback thread is created and owned by CoreAudio; it is already a
  member of the device workgroup.
- Any **additional** realtime worker thread INCDAW creates for parallel graph
  processing must join that workgroup via
  `kAudioDevicePropertyIOThreadOSWorkgroup` and
  `os_workgroup_join` / `os_workgroup_interval_start`.
- Auxiliary realtime work not tied to the device callback uses a workgroup
  created with `AudioWorkIntervalCreate`.

Without this, worker threads on an Apple M5 can be scheduled onto efficiency
cores and cause underruns that appear random and are extremely hard to
diagnose. FL Studio 2026 adopted the same API on macOS for the same reason.

---

## 4. Realtime safety enforcement

Design intent is not enough; the rule must be mechanically checked.

**Debug builds arm a realtime guard on the audio thread.** Entering the audio
callback sets a thread-local flag; a global `operator new` / `operator delete`
override and instrumented lock acquisition assert if that flag is set. Any
allocation or lock on the audio path fails the test suite immediately, in CI,
on the commit that introduced it.

Additional rules, enforced by review and by the guard:

- No `std::shared_ptr` copies on the audio thread (atomic refcount traffic).
- No `std::function` construction (may allocate); use fixed-capacity callables.
- No exceptions thrown or caught across the audio boundary.
- No `std::vector` growth, `std::map`, `std::string`, or any node-based
  container.
- No `printf`, logging, or any syscall.
- Denormals disabled at callback entry (FTZ/DAZ on arm64 via FPCR).
- All buffers preallocated at graph-compile time.

---

## 5. Signal flow

```
                       ┌──────────────┐
   MIDI input ────────►│              │
   (CoreMIDI,          │  Transport   │◄──── tempo map, time signature map
    timestamped)       │  (one clock) │
                       └──────┬───────┘
                              │ sample position
                              ▼
   ┌──────────────────────────────────────────────────┐
   │  RenderGraph  (immutable, topologically sorted)  │
   │                                                  │
   │   Channel ──► Instrument ──► MixerNode ──► ...   │
   │   AudioClip ─────────────►   MixerNode ──► ...   │
   │                                   │              │
   │                            sends / buses         │
   │                                   ▼              │
   │                              Master ─────────────┼──► Audio Logger
   └──────────────────────────────────────────────────┘        (60 s ring)
                              │
                              ▼
                    Audio output device
```

**Transport is a single authority.** MIDI scheduling, audio clip playback,
automation evaluation, and offline rendering all read the same sample position
from the same object. There is no second clock anywhere in the system.

---

## 6. Block processing and sample-accurate events

Each callback renders one block. Events (MIDI notes, automation points, tempo
changes, loop wraps) carry a frame offset within the block. Nodes **split the
block at event boundaries** rather than quantising events to block starts.

This is the difference between a DAW that is sample-accurate and one that merely
claims to be, and it must be true from the first note played in Phase 5 — it
cannot be added later without touching every node.

Loop boundaries and tempo changes are handled by the same block-splitting
mechanism, not by special cases.

---

## 7. Plugin delay compensation

Latency is accumulated at graph-compile time, not at runtime:

1. Each node reports its latency in frames.
2. The compiler computes, for every path to the master output, the total latency.
3. Delay lines are inserted on shorter paths so that all paths reach the master
   with equal delay.
4. The total reported latency is exposed to the UI and to the recording path.

Plugins may change their reported latency while loaded. That triggers a graph
recompile on a background thread and an atomic swap — never an in-place edit of
the graph the audio thread is reading.

---

## 8. Audio Logger

A lock-free ring buffer on the master output retaining the last 60 seconds of
audio, continuously, whether or not recording is armed. FL Studio 2026 ships
this feature ("the Master output keeps the last 60 seconds ready to recover at
any time") and it is genuinely valuable — a performance you did not record is
otherwise simply gone.

Architecturally it is trivial **if designed in now** (one ring buffer write at
the master node, one background thread draining to disk) and invasive if bolted
on later. It is therefore specified here in Phase 0 and implemented in Phase 12.

At 48 kHz stereo float32, 60 seconds costs ~23 MB of resident memory. Acceptable.

---

## 9. Offline rendering

Offline render must use the **same** `RenderGraph` and the **same** node
implementations as realtime playback. The only differences permitted are:

- the driving loop is not paced by a device callback;
- plugins are told they are rendering offline, where their format supports it;
- higher-quality modes may be selected for resampling and time-stretching, and
  when they are, that is a documented, user-visible choice.

A regression test renders a fixed project offline and captures the same project
in realtime, then asserts the results are byte-identical (with any deliberate
quality-mode differences disabled). If that test cannot pass, the architecture
is wrong and must be fixed rather than the test relaxed.

---

## 10. Audio correctness requirements

Per CLAUDE.md §29, the following are tested, not assumed:

- Sample-accurate event timing across block boundaries.
- Correct behaviour at loop points and tempo changes.
- Denormal handling (FTZ/DAZ enabled; no CPU spikes on decaying tails).
- NaN and infinity: detected and contained; one bad plugin must not silently
  poison the master bus.
- Channel count mismatches (mono→stereo, stereo→mono) handled explicitly.
- Sample-rate and buffer-size changes at runtime without a crash or a click.
- Transport seeking mid-playback.
- Offline/realtime equivalence.

---

## 11. Performance budget

On the target machine (Apple M5, 10 cores, macOS 26.2), at 48 kHz / 128 frames,
the callback budget is 2.67 ms.

| Metric | Target |
|---|---|
| Audio callback, empty project | < 1% of budget |
| Audio callback, typical project | < 30% of budget |
| Underruns in a 60-minute soak | **0** |
| Round-trip latency at 128 frames | < 10 ms |

These are targets to be **measured** in Phase 2 with a callback-duration
histogram, not assumed. CLAUDE.md §27 forbids optimising before measuring.
