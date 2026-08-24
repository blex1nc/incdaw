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

> **Status (2026-08-23): Wave 0 is on `main`.** The exact API that landed is
> what §3–§5 below now describe; where it differs from the original sketch
> (`addEffect()` is a header template, registrars are declared in family
> headers, `AssetResolver` has four members), the landed form is the contract.

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

template <class Factory>
void addEffect(std::vector<EffectCatalogueEntry>& rows, const char* uid,
               const char* displayName, Factory factory);   // the probe trick lives here
```

A family owns its own registrar, **declared in its own header** and defined
in its own `.cpp`:

```cpp
// engine/dsp/effects/DistortionEffects.h
struct EffectCatalogueEntry;
void registerDistortionEffects(std::vector<EffectCatalogueEntry>& rows);

// engine/dsp/effects/DistortionEffects.cpp
#include "engine/dsp/effects/EffectRegistry.h"
void registerDistortionEffects(std::vector<EffectCatalogueEntry>& rows)
{
    addEffect(rows, "incdaw.waveshaper", "Waveshaper",
              [](SampleRate) { return std::make_unique<WaveshaperEffect>(); });
}
```

`addEffect()` borrows the parameter table from a throwaway probe so the
catalogue cannot drift from the code — do not bypass it. `BuiltinEffects.cpp`
has **two** marked blocks, and a new family adds one line to each:

```
// <<< incdaw:registrars:effects:include   →  #include "engine/dsp/effects/DistortionEffects.h"
// <<< incdaw:registrars:effects           →  registerDistortionEffects(rows);
```

Catalogue order is registrar-call order; it is also the order the mixer's
insert menu lists effects in, so append your family at the end.

### 3.2 Instruments — `project/InstrumentFactory.h`

The uid if-chain in `ProjectGraphCompiler.cpp` is gone; the compiler calls
`makeBuiltinInstrument()` and warns on nullptr.

```cpp
class AssetResolver {                      // implemented by the compiler; fake it in tests
public:
    virtual const std::string* assetPath(EntityId asset) const = 0;     // nullptr: no such asset
    virtual std::shared_ptr<const engine::AudioFileData> loadAsset(EntityId asset) = 0;
                                           // decoded whole, memoised per compile, warned once if missing
    virtual std::shared_ptr<engine::SamplerZoneStream> streamAsset(EntityId asset) = 0;
                                           // nullptr when the file may not stream → preload instead
    virtual void warn(std::string message) = 0;
};
struct InstrumentBuildContext {
    const Channel&     channel;
    engine::SampleRate sampleRate;
    AssetResolver&     assets;
};
struct BuiltinInstrumentEntry {
    const char* uid;
    std::function<std::unique_ptr<engine::Instrument>(const InstrumentBuildContext&)> make;
};
using InstrumentRegistrar = void (*)(std::vector<BuiltinInstrumentEntry>&);

const std::vector<BuiltinInstrumentEntry>& builtinInstrumentEntries();
const BuiltinInstrumentEntry* findBuiltinInstrumentEntry(std::string_view uid);
std::unique_ptr<engine::Instrument> makeBuiltinInstrument(const InstrumentBuildContext&);
```

`engine::BuiltinInstrumentInfo` (parameter table, `BuiltinInstruments.cpp`)
stays where it is — the registry only adds construction, and a test holds
the two lists to the same uid set. A family is
`project/instruments/<Family>Factory.{h,cpp}` declaring
`void registerXxxInstruments(std::vector<BuiltinInstrumentEntry>&)`; see
`CoreInstrumentFactory.cpp` for the sampler, which is the worked example of
resolving zones through `context.assets`. `InstrumentFactory.cpp` has two
marked blocks (`incdaw:registrars:instruments:include`,
`incdaw:registrars:instruments`).

Whether a *zone* may stream (forward, unlooped) is the instrument's
decision; whether the *file* may (streamer present, long enough, ≤ 2
channels) is the resolver's. Wave 4 adds wavetable/IR/drum-set resolution
as further members — additive.

### 3.3 UI specs — `app/devices/DeviceUiCatalogue.h`

```cpp
using DeviceUiRegistrar = void (*)(std::vector<const DeviceUiSpec*>&);
const std::vector<const DeviceUiSpec*>& deviceUiSpecs();
const DeviceUiSpec* deviceUiSpec(std::string_view uid);   // nullptr -> generic slider panel
```

A family is `app/devices/<Family>Panels.{h,cpp}` declaring
`void registerXxxPanels(std::vector<const DeviceUiSpec*>&)`, which pushes
pointers to function-local statics. `DeviceUiCatalogue.cpp` has two marked
blocks (`incdaw:registrars:panels:include`, `incdaw:registrars:panels`).

A missing spec is legal and falls back to `INCDAWInsertParameterPanel`. Ship
DSP first, spec second, if a wave runs long.

## 4. `DeviceUiSpec` widget vocabulary

Pure data, engine-free, AppKit-free. Agent 1 owns the types; Agents 2/3 only
consume them.

`knob` · `slider` · `fader-wall` · `toggle` · `combo` · `xy-pad` ·
`drawable-curve` · `envelope` · `step-grid` · `pad-grid` · `keyboard` ·
`zone-map` · `scope` · `spectrum` · `goniometer` · `meter` · `matrix` ·
`waveform` · `label` · `section` · `row` · `grid` · `tab`

Each widget names: parameter id(s), range/skew, unit, label, theme token.

**As landed** (`app/devices/DeviceUiSpec.h` — read the header, it is short):

```cpp
enum class DeviceWidget : std::uint8_t { knob, slider, faderWall, toggle, combo, xyPad,
    drawableCurve, envelope, stepGrid, padGrid, keyboard, zoneMap, matrix,
    scope, spectrum, goniometer, meter, waveform, label, section, row, grid, tab };
enum class DeviceSkew : std::uint8_t { linear, logarithmic };
struct DeviceUiRange  { double min, max; DeviceSkew skew; double step; };   // optional per widget
struct DeviceUiWidget {
    DeviceWidget kind;  std::string label;  std::vector<std::uint32_t> parameters;
    std::optional<DeviceUiRange> range;  std::string unit, tint, plot;
    std::vector<std::string> choices;  std::uint16_t columns, rows;
    std::optional<DeviceUiRange> xAxis, yAxis;   // drawn widgets: the plot's axes
    std::vector<DeviceUiPoint>   points;         // drawn widgets: its draggable handles
    bool bipolar, collapsed;  std::vector<DeviceUiWidget> children;
    // chainable: withRange, withUnit, withTint, withPlot, withAxes, withPoints,
    //            asBipolar, startCollapsed
};
struct DeviceUiPoint { std::optional<std::uint32_t> x, y, z; std::string label; };
struct DeviceUiSpec { std::string uid, title; double preferredWidth, preferredHeight;
                      std::vector<DeviceUiWidget> root; std::string customView; };
namespace widgets { knob, slider, toggle, combo, drawn, meter, label,
                    section, row, grid, tab, leaf, container }   // constructors
```

Conventions: `parameters` order is widget-defined (`xyPad` = {x, y},
`envelope` = {a, d, s, r}, `faderWall` = one per fader). Widgets that carry
more than scalars (`stepGrid`, `zoneMap`, `matrix`, extra envelope points)
serialize through the device's `StateIO` blob, not a parameter per cell.
`tint` is a theme ink token by name (`"accent"`, `"midi"`, `"audio"`, …);
`plot` is what a drawn widget shows (`"eq-response"`, `"transfer"`,
`"gate"`, `"sample"`) and is interpreted by the renderer, never by `app/`.
Adjacent `tab` siblings form one tab strip. The Tone panel is expressed as
data in `src/app/devices/TonePanels.cpp` — copy its shape.

**Existence proof for the spec (Agent 1, Wave 1) — DONE 2026-08-24:** the
spec expressed the whole of `TonePanel.mm` (three bands, response curve,
advanced section) without an extension to the vocabulary; the bespoke panel
is deleted and `incdaw.tone` now opens through the renderer. Wave 1 draws
`section`, `row`, `grid`, `label`, `knob`, `slider`, `fader-wall`, `toggle`,
`combo`, `meter` and the `eq-response` curve. **A spec may name a widget the
renderer does not draw yet** — it lays out as a labelled well and the panel
still opens, so Agents 2 and 3 need not wait for Wave 2.

**Vocabulary extension (Agent 1, Wave 2, 2026-08-24) — additive, nothing
renamed or removed.** A drawn widget can now say where its handles are:

- `DeviceUiPoint { optional<uint32_t> x, y, z; string label; }` — one
  draggable handle. Each axis names the parameter that axis writes; an axis
  the point leaves unset does not move, and the handle sits centred on it.
  `z` is the SCROLL axis (a band's Q, a point's tension). The ids are
  `optional` rather than 0-as-sentinel because 0 is a real parameter id.
- `DeviceUiWidget::xAxis` / `yAxis` — the ranges, with skew, the widget is
  plotted against. Unset takes the renderer's own defaults for that `plot`.
  **State them if the widget has points:** the renderer draws its grid and
  its curve on exactly these, so stating them is what keeps a handle on the
  line it belongs to.
- `DeviceUiWidget::points` — the handles, in drawing order.
- Chainable `withAxes(horizontal, vertical)` and `withPoints({...})`.

The arithmetic is `app/devices/DeviceUiPlot.h`, and it is where a panel's
gestures are tested without a window server: `plotAxes` resolves the pair,
`handleRect` places a handle, `handleAt` grabs the nearest one within
`plot::grabRadius`, `handleDrag` returns one already-constrained
`DeviceUiWrite` per axis, and `handleScroll` moves `z` by
`plot::scrollTravel` of its own range per tick. A point naming an id the
device does not carry makes the WHOLE spec fall back to the generic panel,
the same rule `parameters` already had — a dead handle never ships.

`incdaw.tone` is the worked example: `.withAxes(20 Hz…20 kHz log, ±18 dB)`
and three points, the mid band carrying `midQ` as its `z`.

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
| `src/app/devices/DeviceUiCatalogue.cpp` | owner | append block (×2) | append block (×2) |
| `src/engine/dsp/effects/EffectRegistry.h` | **owner** | ✗ | ✗ |
| `src/engine/dsp/effects/BuiltinEffects.cpp` | owner | append block (×2) | ✗ |
| `src/project/InstrumentFactory.{h,cpp}` | owner | ✗ | append block (×2, .cpp only) |
| `src/project/ProjectGraphCompiler.cpp` | **owner** | ✗ | ✗ |
| `src/CMakeLists.txt` | owner | `incdaw:sources:engine-effects`, `incdaw:sources:app-panels` | `incdaw:sources:engine-instruments`, `incdaw:sources:project-instruments`, `incdaw:sources:app-panels` |
| `tests/CMakeLists.txt` | owner | `incdaw:tests:effects` | `incdaw:tests:instruments` |
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
