# INCDAW Plugin Archive — Shared Contracts

Read this before touching anything. All three agents work against it.

Target: **68 built-in devices** (48 effects + 20 instruments) from today's 20
(17 effects + 3 instruments). Split into three parallel work packages:

| Agent | Package | Branch |
|---|---|---|
| 1 | Device framework, preset system, panel renderer, registries | `feat/plugin-framework` |
| 2 | Effects archive (+31) | `feat/plugin-effects` |
| 3 | Instruments archive (+17) | `feat/plugin-instruments` |

---

## 1. Sequencing — the contract freeze gate

Agents 2 and 3 **cannot start** until Agent 1 lands **Wave 0: the contract
freeze** on `main`. Wave 0 is headers only — no DSP, no panels, no presets:

- `src/app/devices/DeviceUiSpec.h`
- `src/app/devices/DeviceUiCatalogue.h`
- `src/engine/dsp/effects/EffectRegistry.h`
- `src/project/InstrumentFactory.h`
- `src/app/DevicePreset.h`
- the registrar refactor of `BuiltinEffects.cpp` and of the instrument
  if-chain in `ProjectGraphCompiler.cpp:455-461` (behaviour identical, all
  existing tests still green)

Wave 0 is small and must be one commit. After it is on `main`, all three
agents run concurrently.

---

## 2. Layering — where each half of a device lives

`ui -> app -> project -> engine -> platform` (enforced by
`tools/check_layering.py`). One device is therefore **three files in three
layers**, never one:

| Part | Layer | Path |
|---|---|---|
| DSP + parameter table | engine | `engine/dsp/effects/<Family>Effects.{h,cpp}` · `engine/instrument/<Name>.{h,cpp}` |
| Factory / asset resolution | engine (effects) · project (instruments) | `EffectRegistry` registrar · `project/instruments/<Family>Factory.cpp` |
| UI spec (declarative, no AppKit) | app | `app/devices/<Family>Panels.cpp` |

The engine never learns what a knob is. The UI never learns what a biquad is.
`app/devices/*` is pure data: it names widgets and parameter ids.

---

## 3. The registries

### 3.1 Effects — `engine/dsp/effects/EffectRegistry.h`

```cpp
struct EffectCatalogueEntry {
    BuiltinEffectInfo info;
    std::function<std::unique_ptr<BuiltinEffect>(SampleRate)> make;
};
using EffectRegistrar = void (*)(std::vector<EffectCatalogueEntry>&);
```

`BuiltinEffects.cpp` keeps its `add()` helper (it still borrows the parameter
table from a throwaway probe — do not change that) and calls one registrar per
family inside a marked block. A family owns its own `registerXxxEffects()`.

### 3.2 Instruments — `project/InstrumentFactory.h`

The uid if-chain in `ProjectGraphCompiler.cpp` does not scale to 20
instruments. It becomes a registry:

```cpp
struct InstrumentBuildContext {
    const Channel&   channel;
    SampleRate       sampleRate;
    AssetResolver&   assets;      // sample/wavetable/IR decode, off the audio thread
};
struct BuiltinInstrumentEntry {
    const char* uid;
    std::function<std::unique_ptr<engine::Instrument>(const InstrumentBuildContext&)> make;
};
```

`engine::BuiltinInstrumentInfo` (parameter table) stays where it is — the
registry only adds construction.

### 3.3 UI specs — `app/devices/DeviceUiCatalogue.h`

```cpp
const DeviceUiSpec* deviceUiSpec(std::string_view uid);   // nullptr -> generic slider panel
```

A missing spec is legal and falls back to `INCDAWInsertParameterPanel`. Ship
DSP first, spec second, if a wave runs long.

---

## 4. `DeviceUiSpec` widget vocabulary

Pure data, engine-free, AppKit-free. Agent 1 owns the types; Agents 2/3 only
consume them.

`knob` · `slider` · `fader-wall` · `toggle` · `combo` · `xy-pad` ·
`drawable-curve` · `envelope` · `step-grid` · `pad-grid` · `keyboard` ·
`zone-map` · `scope` · `spectrum` · `goniometer` · `meter` · `matrix` ·
`waveform` · `label` · `section` · `row` · `grid` · `tab`

Each widget names: parameter id(s), range/skew, unit, label, theme token.

**Existence proof for the spec (Agent 1, Wave 1):** the spec must be able to
express the whole of today's `TonePanel.mm` (three bands, response curve,
advanced section). If it cannot, the vocabulary is short — extend it before
Agents 2/3 build 60 panels on it.

**Escape hatch:** `DeviceUiSpec::customView` names a bespoke Objective-C++
view for surfaces the vocabulary genuinely cannot carry. Budget: **≤ 8
devices total** across the whole archive. Every use is justified in
`docs/DECISIONS.md`.

---

## 5. File ownership — who may edit what

Shared files carry sentinel comments. Append **inside your own marked block
only**, never reorder another agent's lines.

```
// <<< incdaw:registrars:effects — one line per family
// >>> incdaw:registrars:effects
```

| File | Agent 1 | Agent 2 | Agent 3 |
|---|---|---|---|
| `src/ui/macos/main.mm` (5020 lines) | **owner** | ✗ never | ✗ never |
| `src/ui/macos/DevicePanel.{h,mm}` | **owner** | ✗ | ✗ |
| `src/app/devices/DeviceUiSpec.h` | **owner** | ✗ | ✗ |
| `src/app/devices/DeviceUiCatalogue.cpp` | owner | append block | append block |
| `src/engine/dsp/effects/BuiltinEffects.cpp` | owner | append block | ✗ |
| `src/project/InstrumentFactory.cpp` | owner | ✗ | append block |
| `src/project/ProjectGraphCompiler.cpp` | **owner** | ✗ | ✗ |
| `src/CMakeLists.txt` | owner | append block | append block |
| `tests/CMakeLists.txt` | owner | append block | append block |
| new family files under your own package | — | yours | yours |

`main.mm` is deliberately closed to Agents 2 and 3: once the panel dispatch is
spec-driven, **no device needs shell wiring**. If you think yours does, that
is a framework gap — report it to Agent 1, do not patch the shell.

---

## 6. Per-device Definition of Done

A device is done when **all ten** hold (CLAUDE.md §44):

1. Catalogue/registry entry with a stable `incdaw.<uid>`.
2. Parameter table with sane defaults, ranges, units, `stepped` flags.
3. State round-trips through the id-keyed `StateIO` blob.
4. Every parameter appears in `ParameterRegistry` (automatable + MIDI-mappable).
5. **Null/unity test**: at default settings the effect either nulls bit-exactly
   against its input or its deviation is stated and tested. Instruments:
   silence with no MIDI, deterministic output for a fixed note.
6. DSP correctness test (analytic or golden) — not "it produces audio".
7. Realtime safety: no allocation, no lock, no I/O in `process`/`renderRange`.
   Covered by `RealtimeSafetyTests` and `FuzzTests` (NaN/Inf/denormal input).
8. UI spec (or a justified custom view) that renders and writes live values.
9. At least 4 factory presets that are musically distinct.
10. One line in `docs/FL2026_GAP.md` §3 flipping the gap to covered, plus the
    `CHANGELOG.md` entry.

No placeholder DSP. A device that "looks right" but is a one-pole filter
wearing a reverb label does not count (CLAUDE.md, development philosophy).

---

## 7. Naming and IP

Original names only. No Image-Line marks ("Fruity", "FLEX", "Gross Beat",
"Maximus", "Sytrus", "Harmor", "Slicex", …) anywhere in uids, display names,
preset names, comments or docs. FL Studio is a **functional** reference:
implement the capability, never the implementation, assets or branding
(CLAUDE.md §43).

Assets (wavetables, impulse responses, drum samples) are **generated or
originally recorded**. No sample-library imports (§20).

---

## 8. Build, test, verify

```sh
cmake -S . -B build -G Ninja
cmake --build build > /tmp/build.log 2>&1; tail -40 /tmp/build.log
(cd build && ctest --output-on-failure)
python3 tools/check_layering.py
```

Never pipe `cmake --build` into `head` — ninja dies on SIGPIPE and the tests
then report a phantom failure. Redirect to a file.

`-Werror` is on. Zero warnings, or the wave is not done.

## 9. Merge protocol

- One wave = one PR into `main`. Never merge a half-wave.
- Rebase onto `main` before every wave merge; resolve sentinel blocks by
  keeping **both** sides.
- After a merge: `graphify update .`, then push (`git push origin main`).
- Never use `graphify label` — it re-clusters and deletes `graph.html`.
- Scope expansion is reported, not fixed (CLAUDE.md §40).

---

## 10. Cross-agent dependencies

Some devices need framework work that lands later than the wave they sit in.
None of them is a reason to stall — each has a stated way to make progress now.

| Device | Waits on | What to do meanwhile |
|---|---|---|
| `incdaw.envfollow` (E1) | Agent 1 Wave 4 — modulation-source plumbing | Ship the detector DSP, parameters, state and metering panel as a normal insert. Wire the output as a modulation source when Wave 4 lands; that is an additive change, not a rewrite. |
| `incdaw.formula` (E4) | Agent 1 Wave 4 | Same: compile-and-evaluate first, expose as a source afterwards. E4 is late enough that Wave 4 will likely already be in. |
| `incdaw.convolver` (E3) | Agent 1 Wave 4 — `AssetResolver` for impulse responses | Build against a procedurally generated IR (a decaying noise burst synthesized in `prepare`) so the partitioned FFT and its PDC reporting are testable. Swap to loaded IRs when the resolver lands. |
| `incdaw.wavetable` (I1) | Agent 1 Wave 4 — wavetable asset tool | **Not blocking.** Factory tables are synthesized procedurally in `prepare` (band-limited saw/square/formant sweeps). The resolver is only needed for *user-supplied* tables — a later additive step. |
| `incdaw.drumpad` (I2) | Agent 1 Wave 4 — drum sample set | Build and test against sine/noise bursts generated in the test fixture. The zone engine is the real work and does not depend on which samples are in it. |
| `incdaw.rompler` (I3) | Agent 1 Wave 3 — preset system | I3 is the last instrument wave; Wave 3 should already be merged. If it is not, build the multisample engine and macros, and leave preset browsing for a follow-up commit. |
| `incdaw.sfz`, `incdaw.slicer`, `incdaw.granular` | `AssetResolver` | Same pattern: fixture-generated audio for tests, resolver wired when available. |
| Any device panel (both agents) | Agent 1 Waves 1–2 — widget vocabulary | A missing spec is legal (§3.3): the device falls back to the generic slider panel. Ship DSP + tests, add the spec when the widget exists. |

Rule: **a framework gap never blocks DSP.** If it seems to, report it to Agent
1 rather than working around the contract.
