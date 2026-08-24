# Agent 1 — Device Framework

Branch: `feat/plugin-framework`. Read `00-CONTRACTS.md` first; you own most of
what it specifies.

You build the layer the other 66 devices stand on. You write **no DSP**.

---

## Wave 0 — Contract freeze  *(blocks Agents 2 and 3 — do this first, one commit)*  — **DONE 2026-08-23**

Headers and a behaviour-preserving refactor. No features.

1. `src/app/devices/DeviceUiSpec.h` — the widget vocabulary of §4, as plain
   data. No AppKit, no engine types. A spec is a tree of sections/rows/grids
   whose leaves name parameter ids.
2. `src/app/devices/DeviceUiCatalogue.{h,cpp}` — `deviceUiSpec(uid)` plus the
   registrar sentinel block. Returns nullptr today.
3. `src/engine/dsp/effects/EffectRegistry.h` — `EffectCatalogueEntry`,
   `EffectRegistrar`. Refactor `BuiltinEffects.cpp` so the existing 17 effects
   register through five family registrars (dynamics, tone, space, modulation,
   utility). The probe-borrows-the-parameter-table trick stays exactly as is.
4. `src/project/InstrumentFactory.{h,cpp}` — `InstrumentBuildContext`,
   `BuiltinInstrumentEntry`, registry lookup. Move the uid if-chain at
   `ProjectGraphCompiler.cpp:455-461` behind it. `AssetResolver` is an
   interface over what the compiler already does to decode samples.
5. `src/app/DevicePreset.h` — the preset object's shape only.

**Exit:** all 740 existing test cases green, zero warnings,
`tools/check_layering.py` clean, no behaviour change anywhere. Merge and push
immediately — two agents are waiting on it.

---

## Wave 1 — The panel renderer  — **DONE 2026-08-24**

`src/ui/macos/DevicePanel.{h,mm}` — one Objective-C++ view that renders any
`DeviceUiSpec`.

- Same discipline as `INCDAWInsertParameterPanel`: the panel holds **row data
  and a write block, never an engine pointer**. Sinks die on every graph
  rebuild; the shell re-resolves the slot on each write.
- `refreshWindow:values:` follows automation, MIDI knobs and undo; ignored
  while the mouse is down so a refresh never fights a drag.
- `refreshAppearance:` reassigns theme colours — an AppKit label's `textColor`
  is a snapshot, not a binding.
- Widgets to land in this wave: `section`, `row`, `grid`, `label`, `knob`,
  `slider`, `fader-wall`, `toggle`, `combo`, `meter`.

**Existence proof:** re-express `TonePanel.mm` (three bands + response curve +
advanced section) purely as a spec. Keep `TonePanel.mm` in the tree until the
spec version is pixel-plausible and its tests pass, then delete it and remove
its dispatch from `main.mm:2562`. If the spec cannot express it, extend the
vocabulary — that is what this wave is for.

**Wire the dispatch:** `main.mm` "Open Editor" becomes: spec found → device
panel; no spec → generic parameter panel; hosted plugin → its own editor.
After this, **no device ever needs a line in `main.mm` again.**

### What landed

- `app/devices/DeviceUiLayout.{h,cpp}` — the geometry, in points, with no
  AppKit. The renderer draws and hit-tests rectangles it does not compute,
  which is what lets a panel be checked without a window server.
- `app/devices/DeviceUiValue.{h,cpp}` — travel↔value (linear and log skew),
  clamping, the stepped round, the bipolar zero detent, and the readout text.
  Every panel in the archive shares this arithmetic.
- `ui/macos/DevicePanel.{h,mm}` — the renderer. Row data and a write block,
  never an engine pointer. Also `INCDAWDeviceCustomView`, the protocol behind
  the `customView` escape hatch.
- `app/devices/TonePanels.cpp` — `incdaw.tone` as data. The only spec Agent 1
  owns; Agents 2 and 3 own theirs.
- `main.mm` dispatch now reads `app::deviceUiSpec(uid)` and names no device.
  `TonePanel.{h,mm}` deleted. The palette walk calls
  `[INCDAWDevicePanel refreshAppearance:]` alongside the generic panel's.
- Tests: `tests/unit/DeviceUiTests.cpp` (fits its width, nothing overlaps,
  folding shortens the panel, a grid keeps one baseline and wraps, travel
  round-trips, the detent, the readouts, every Tone control drives a real
  parameter); `DeviceFrameworkTests.cpp` repointed from the inline spec to
  the registered one. 759 cases green, zero warnings, layering clean.

**Not verified:** pixel parity against the deleted `TonePanel.mm` — the app
launches and the panel lays out as the tests state, but no side-by-side
screenshot was taken.

**Deferred, on purpose:** `openInstrumentPanelForChannel:` still uses the
generic panel. Instruments get specs when Agent 3 writes them; the dispatch
there is a one-line change then.

## Wave 2 — Drawn and interactive widgets

**Increment 1 — the draggable curve — DONE 2026-08-24.** `drawable-curve` is
now a control, not a picture. What landed:

- Vocabulary extension (published in `00-CONTRACTS.md` §4): `DeviceUiPoint`
  with `x`/`y`/`z` parameter ids, `DeviceUiWidget::xAxis`/`yAxis`/`points`,
  and the `withAxes` / `withPoints` modifiers. Additive; nothing renamed.
- `app/devices/DeviceUiPlot.{h,cpp}` — the AppKit-free half: `plotAxes`
  resolves the widget's axes over the renderer's defaults, `axisPositionX/Y`
  and `axisValueX/Y` are the two directions of one arithmetic, `handleRect`
  places a handle, `handleAt` grabs the nearest inside `plot::grabRadius`,
  `handleDrag` returns one constrained `DeviceUiWrite` per axis, and
  `handleScroll` moves `z` by `plot::scrollTravel` of its own range per tick.
- `ui/macos/DevicePanel.mm` — the grid, the curve and the handles are drawn
  from the SAME resolved axes; drag moves a band, ⌥-free vertical is its
  gain, the wheel over a handle is its Q, double-click resets the axes the
  point moves, and a scroll that grabs nothing is passed to the scroll view.
  The panel's validation walk now covers `points` as well as `parameters`,
  so a spec naming an id the device lacks falls back rather than shipping a
  dead handle.
- `incdaw.tone` states ±18 dB and three handles; the mid band carries `midQ`.
- Tests: four cases in `tests/unit/DeviceUiTests.cpp` (axes resolution and
  handle placement, a drag writing both axes inside the table and pinning at
  the ends, the wheel's tick, nearest-wins hit testing). 763 cases green,
  zero warnings, layering clean.

**Still to come in this wave:** `xy-pad`, `envelope`, `step-grid`,
`pad-grid`, `keyboard`, `zone-map`, `scope`, `spectrum`, `goniometer`,
`matrix`, `waveform` — and the lock-free snapshot the displays need.


`drawable-curve` (EQ curve, transfer curve, gate curves — draggable points,
tension) · `xy-pad` (with recordable path) · `envelope` (ADSR + multipoint) ·
`step-grid` · `pad-grid` · `keyboard` · `zone-map` · `scope` · `spectrum` ·
`goniometer` · `matrix` · `waveform`.

Each widget reads and writes plain parameter values through the same write
block. Curves and grids that carry more than a scalar serialize through the
device's own `StateIO` blob, not through a parameter id per point.

Performance: a panel must repaint within the UI frame budget in
`docs/PERFORMANCE.md` §2 with 8 open panels. Scopes and spectra draw from a
lock-free snapshot the audio thread publishes — never by reading engine state
under a lock.

## Wave 3 — Preset system  *(gap A5)*

- `app::DevicePreset` — uid, name, author, tags, the device's state blob.
- Factory bank shipped with the app; user bank under the app support dir.
- Commands: save, save-as, load, rename, delete, favourite — all through
  `CommandRegistry`, all undoable where they change project state.
- `app::Browser` already classifies files and does folders-first listings,
  favourites, recents, drag and drop. Presets become a browser category and
  drag onto a channel or an insert slot.
- **Project format bump to v1.6** with a migration: a project referencing a
  preset must load in an older-format-aware way, and v1.5 projects must load
  unchanged. Round-trip test both directions.

## Wave 4 — Asset pipeline and modulation sources

- `AssetResolver` implementations for wavetable tables, impulse responses and
  drum sample sets — decoded off the audio thread, cached through the existing
  `SampleCache`, hot-swapped into a rebuilt graph like any other asset.
- All shipped assets are **generated by a tool in `tools/`** (a script that
  synthesizes the wavetables and IRs) so the repo carries a reproducible
  recipe, not an opaque binary blob (§20, §43).
- Modulation-source plumbing: a device output (envelope follower, formula, XY
  macro) targets any registered parameter through `ParameterRegistry` and the
  existing automation path. No new automation system — CLAUDE.md §10 forbids
  per-plugin automation.

---

## Reporting

Publish to the other two agents, in `docs/plugin-archive/00-CONTRACTS.md`:
every vocabulary extension, every contract change. A silent change to
`DeviceUiSpec.h` breaks 60 panels being written in parallel.
