# INCDAW — Testing Strategy

Status: **Phase 19 — the suite is live: 483 cases, ~1.34M assertions, green
in Debug and Release.** It includes per-phase exit-criterion tests, permanent
format fixtures (v1.0–v1.4), deterministic fuzzing of every byte-level reader
(project package, WAV, SMF), large-project stress, and a long-session
edit-rebuild-process loop (tests/unit/FuzzTests.cpp, StressTests.cpp).
The strategy below is the standing contract each phase built against.

---

## 1. Principle

A feature is not done because it compiles or because the UI shows something
(CLAUDE.md §44). It is done when its behaviour is proven by a test that would
fail if the behaviour regressed.

Two consequences INCDAW takes literally:

- **Every serious bug gets a permanent regression test** (§28). No exceptions.
- **The core is testable headlessly** — no UI, no audio device, no plugins
  required to run the majority of the suite.

---

## 2. Framework

**doctest** (MIT, header-only) — chosen for compile speed and zero build
complexity, and because its license is compatible with closed-source INCDAW
(D-008). Catch2 (BSL-1.0) is an acceptable alternative. Adoption is a pending
dependency approval.

---

## 3. Test levels

### Unit — the bulk of the suite

Headless, fast, no device. Priority order follows CLAUDE.md §28:

1. Audio engine primitives, graph compilation, PDC arithmetic
2. Transport: position, tempo map, loop, seek
3. MIDI: events, quantize, humanize, tempo-aware conversion
4. Project serialization: round-trip, determinism, migration
5. Automation: interpolation, curves, tension
6. Routing: topology, cycle detection, latency accumulation
7. Plugin host: registry, parameter mapping, state round-trip
8. DSP: per-block correctness
9. UI state (headless view models)
10. Rendering/export

### Integration

Audio engine + mixer · MIDI + instruments · plugins + mixer · playlist +
transport · project save/load with a full session.

### End-to-end

Create project → add instrument → program MIDI → arrange → mix → automate →
render. Run as one scripted test through the **command registry**, which is
possible precisely because every action is a command (docs/ARCHITECTURE.md §6).

---

## 4. Specialised tests

### Realtime safety (from Phase 2)

Debug builds arm a thread-local guard on the audio callback. A global
`operator new`/`delete` override and instrumented locks assert if triggered
inside the callback. **Any allocation or lock on the audio thread fails the
suite on the commit that introduced it.**

### Soak (from Phase 2)

60 minutes of continuous playback. Asserts: zero underruns, no memory growth,
callback duration histogram within budget.

### Golden-file audio (from Phase 7)

A fixed project renders to WAV and is compared against a checked-in reference.
Catches DSP regressions that no unit test would notice. References are
regenerated only with a deliberate, reviewed commit that explains the change.

### Render equivalence (Phase 17 gate, written earlier)

Offline render must be **byte-identical** to a realtime capture of the same
project. If this fails, the architecture is wrong; the test is not relaxed.

### Layering (from Phase 1)

Static check over the source tree: `engine/` must not reference `ui/` or `app/`;
macOS and platform symbols must appear only in `platform/`; no circular
dependencies between layers. This is what turns docs/ARCHITECTURE.md §2 from a
document into a constraint.

### Project format fixtures (from Phase 4)

One fixture per released format version, kept forever. A migration that breaks
an old fixture fails CI.

### Fuzzing (from Phase 4)

Randomly corrupted and truncated project files must be rejected cleanly — never
crash, never silently lose data. Extended to plugin scanning in Phase 13.

### Plugin misbehaviour matrix (Phase 13 gate)

Deliberately hostile test plugins that: crash on process, hang on load, return
NaN, report absurd latency, allocate on the audio thread, and fail state
restore. **INCDAW must survive every one with the project intact.** Real
third-party plugins (including those installed on this machine) supplement this
with real-world integration testing.

### Performance regression (from Phase 2)

Callback duration, UI frame time and project load time are measured in CI and
compared against recorded baselines. A significant regression fails the build.

---

## 5. What is not tested automatically

Honesty matters more than coverage theatre:

- **Audio quality judgements** — null tests and spectral comparison catch
  regressions, not whether a reverb sounds good.
- **Plugin editor rendering** — hosted third-party UIs cannot be meaningfully
  asserted on; verified manually.
- **Device-specific behaviour** — only the hardware present can be tested.
- **Windows** — no builds exist (D-005). The layering test is the only thing
  currently protecting portability, and it is not a substitute for compiling.

These gaps are stated here so that no one later mistakes a green suite for
proof they were covered.
