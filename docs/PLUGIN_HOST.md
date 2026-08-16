# INCDAW — Plugin Host

Status: **Phase 0 — design only. Not implemented.**
Target: Phase 13.

---

## 1. Supported formats

Decision D-007. Implementation order is deliberate:

| Order | Format | License | Rationale |
|---|---|---|---|
| 1 | **CLAP** | MIT | Cleanest ABI, sample-accurate events, explicit threading contract. Best format to build the host architecture on. |
| 2 | **Audio Units (v2 + v3)** | Apple SDK | Native to macOS, mandatory there. Headers verified present. |
| 3 | **VST3** | MIT (SDK 3.8.x) | Widest plugin availability. Steinberg withdrew GPLv3/proprietary licensing; MIT is compatible with closed-source INCDAW. |
| — | **VST2** | *not licensable* | **Excluded.** Steinberg ended VST2 SDK licensing in 2018; no lawful path for a new closed-source product. |

This matches FL Studio's macOS format list (VST 1/2, VST3, AU, CLAP) with the
single, legally-forced exception of VST2.

---

## 2. Prime directive

**Third-party plugins are hostile input.** They crash, hang, allocate on the
audio thread, report wrong latency, return NaN, open windows on the wrong
thread, and misbehave on unload. The host is not permitted to assume otherwise.

Every DAW that has a reputation for instability earned it here.

---

## 3. Pipeline

```
PluginScanner  ─►  PluginRegistry  ─►  PluginInstance  ─►  RenderGraph node
     │                   │                    │
  out-of-process     persisted           ParameterBridge
  sandbox, per-      cache + blacklist    StateBridge
  plugin timeout                          EditorBridge
                                          LatencyReporter
```

### PluginScanner

- Runs **out-of-process**, one plugin at a time, with a hard timeout.
- A plugin that crashes or hangs the scanner is recorded in the **blacklist**
  and skipped on subsequent scans; it never blocks a later scan or startup.
- Results are cached with the plugin's path, size, mtime and format; rescans
  only re-examine changed entries.
- The blacklist is user-visible and user-clearable, and rescan can be forced.

### PluginRegistry

Persisted catalogue: identifier, format, path, name, vendor, category, I/O
configuration, and cached parameter list. Loaded at startup without touching a
single plugin binary — startup time must not scale with the size of the user's
plugin collection.

### PluginInstance

Wraps one loaded plugin behind a format-agnostic interface. The engine knows
only this interface; nothing above `plugins/` ever sees a CLAP, AU, or VST3
type.

### PluginInstanceManager

Owns the loaded libraries for the application's lifetime, keyed by path. A
`PluginNode` owns its instance, but destroying that instance calls back into
the library it came from, and graphs are rebuilt on every edit and retired
asynchronously — a library owned by a graph would be unloaded while a node
from it was still queued for destruction. Two instances of the same plugin
therefore share one opened binary, and no binary is closed while the
application runs.

### Inserts in the graph

A `MixerNode`'s `inserts` are compiled into a chain in front of its strip:

    incoming edges → insert[0] → insert[1] → … → strip (fader, pan, mute)

Inserts run **pre-fader** (docs/DECISIONS.md D-028): a fader move must not
change what a compressor hears. The compiler keeps two index maps per mixer
node — where signal *enters* (the head of the chain) and where it *leaves*
(the strip) — because a send routed into the destination's fader would
bypass its plugins.

`project/` never includes a plugin header. The compiler asks
`GraphCompileOptions::insertFactory` for an `engine::Node` per slot, and the
shell supplies a factory backed by the instance manager. A bypassed slot is
not instantiated at all; a slot the factory cannot build becomes a
pass-through and a named compile warning, never silence and never a failed
compile.

---

## 4. Isolation strategy

**The host ABI is designed for out-of-process hosting from its first line of
code**, even though Phase 13 implements in-process hosting first. Retrofitting
isolation into an in-process design means rewriting the host, so the boundary is
drawn now:

- Audio crosses the boundary through a **shared-memory ring buffer**, not
  function calls.
- Parameter changes, state, and events cross as **serialisable messages**.
- The interface exposes **no raw pointers into INCDAW's address space**.

This means the in-process implementation is a degenerate case of the
out-of-process one, and switching a plugin (or all plugins) to sandboxed mode
later is a configuration change rather than a redesign.

**Crash policy:** a plugin process that dies is detected by a watchdog. The
affected node is replaced with a silent pass-through, the user is notified, the
project stays open, and the plugin's last-known state is retained so it can be
restored on reload. Losing a session to someone else's bug is not acceptable.

---

## 5. Parameter system

Implemented (docs/DECISIONS.md D-029):

- Discovery happens once per plugin TYPE, at first instantiation, on the main
  thread: `ClapInstance` snapshots `CLAP_EXT_PARAMS` into format-agnostic
  `plugins::PluginParameterInfo` (plain min/max/default, stepped flag;
  non-automatable and malformed parameters are skipped as hostile input).
  `PluginInstanceManager` caches the list per uid.
- Every parameter maps to INCDAW's **generic** automation subsystem — there is
  no plugin-specific automation code anywhere (CLAUDE.md §10). The
  `ParameterRegistry` entry generalised: `StripApplier` OR `SinkApplier` over
  `engine::ParameterSink`. Registration
  (`registerPluginParameters`) is all a plugin's parameters need to become
  automatable; keys are `plugin:<uid>:<param-id>`, and the lane's
  `targetEntity` (the insert slot id) picks the instance.
- Delivery is by EVENT, never by a call into the plugin: the instance is a
  `ParameterSink` whose `setParameter` pushes onto a preallocated lock-free
  queue; `process()` drains it into the block's `clap_event_param_value`
  input list at block start. This is what keeps the CLAP concurrency contract
  (no flush during processing) unviolatable by construction.
- The graph compiler binds a sink-applier lane to the insert node's
  `parameterSink()`; a mismatched key/target pair is skipped as data, exactly
  like an unknown key.

Still to come:

- `params->flush()` while idle — RECORDED AS NOT CURRENTLY APPLICABLE
  (2026-08-16): every code path that writes a hosted instance's parameters
  (panel, automation, MIDI map) resolves through the compiled graph, and a
  slot without a live node in the graph has no sink to write through — so a
  value can never sit in the queue of an instance whose process() is not
  running. The case returns if a future surface writes to a BYPASSED slot's
  persisted instance directly; implement flush then, not before.
- Plugin-originated parameter changes (a user turning a knob in the plugin's
  own editor) flow back through the output event list and are recordable as
  automation (§7).
- Sample-accurate parameter changes are honoured where the format supports
  them (CLAP natively; VST3 via sample-offset parameter queues). Today the
  grain is one block, matching the AutomationNode's evaluation.

---

## 6. State

The isolated `Plugin State System` node flagged by Graphify as under-specified
in the constitution is defined here.

Plugin state is an **opaque binary blob** owned by the plugin, stored per
instance in the project package under `plugins/` (docs/PROJECT_FORMAT.md).
INCDAW never interprets it.

Implemented (docs/DECISIONS.md D-030): `ClapInstance` is an `engine::StateIO`
(CLAP_EXT_STATE through stack-local stream adapters, 64 MB save cap against
hostile plugins); the compiled graph maps each live insert slot to its
carrier (`insertStateFor`); `project/PluginStateFiles` captures blobs into
`plugins/insert-<slot-id>.state` (stage-and-rename) and restores them after a
compile. `PluginSlot::stateFile` records the package-relative path and
already travels in project.json. Capture runs BEFORE `ProjectFile::save`;
restore runs after compiling a loaded project. The shell does not call either
yet — the application still has no save/open action (the standing gap).

Rules:
- State is saved on project save and on freeze/bounce.
- State survives a plugin crash: the last successfully-saved blob is retained.
  (Enforced twice: a failed `save()` keeps the previous blob and stateFile,
  and blob writes stage-and-rename so a crash mid-write loses only the new
  version.)
- A missing plugin at load time does **not** discard its state — the node
  becomes a placeholder that retains the blob, so installing the plugin later
  restores the session intact. (Enforced by capture/restore only touching
  slots with a live carrier.)
- A blob the plugin rejects is a named warning and the plugin plays its
  defaults; the blob stays on disk.
- State is versioned alongside the project format so a future migration can
  reason about it: blobs live inside the versioned package, and the manifest
  records which app version saved them.

---

## 7. Editor / UI bridge

Implemented (part 10):

- `ClapInstance` bridges CLAP_EXT_GUI for embedded (non-floating) Cocoa
  editors: create → set_scale → get_size → set_parent → show, with no editor
  left behind on any refusal; hide → destroy on close; the destructor closes
  an open editor first, so a closing window and a dying instance cannot
  double-free. The parent view crosses the layer as void* — plugins/ never
  includes Cocoa.
- The shell owns one NSWindow per slot (Open Editor in the mixer's insert
  submenu), keyed by slot id. Because instances live for their slot's
  lifetime (D-031), the window survives graph rebuilds; when a slot leaves
  the project, the shell closes its window BEFORE the retain pass disposes
  the instance.

Still to come:

- Plugin editors are hosted in a native window owned by `ui/`, via
  `CoreAudioKit` / `AUCocoaUIView` for AU and the format-native mechanisms for
  CLAP and VST3.
- **Editor code never runs on the audio thread**, and the audio thread never
  waits on an editor.
- A plugin that hangs its editor must not hang INCDAW's UI thread — in
  out-of-process mode this is structural; in in-process mode it is a known,
  documented limitation.

---

## 8. Latency reporting

Implemented:

- Each instance reports its latency in frames: `ClapInstance` queries
  CLAP_EXT_LATENCY once, while activated, at creation. Hostile input is
  capped at ten seconds — an absurd report must not make PDC build a giant
  delay line on every parallel path.
- Latency feeds PDC at graph-compile time through the ordinary channel:
  `PluginNode::latencyFrames` is just a node's latency, and the graph's
  existing compensation (docs/AUDIO_ENGINE.md §7) aligns parallel paths with
  no plugin-specific code in the engine. Proven end to end against the test
  suite's own truly-latent plugin (a 64-frame delay that reports itself).

Still to come:

- DONE (2026-08-16, UI build-out increment 5): clap_host_latency.changed
  lands on an atomic flag on the instance; the shell's housekeeping consumes
  the flags (PluginInstanceManager::refreshChangedLatencies), re-reads the
  figure under the creation-time hostile cap, and recompiles — the same
  atomic swap every edit uses. Proven end to end against the latency test
  plugin, whose state load doubles its report and rings the callback.
- Plugins that report **wrong** latency are a known real-world problem; a
  manual per-instance latency offset is provided as an escape hatch.

---

## 9. Validation

Before a plugin is trusted in a user's project it is checked for:

- I/O configuration sanity.
- NaN/Inf output on silence and on known input.
- Allocation on the audio thread (detectable in the sandbox).
- Behaviour on unload and reload.
- State save/restore round-trip fidelity.

Failures are recorded in the registry and surfaced to the user rather than
silently tolerated.

---

## 10. Testing

Per docs/TESTING.md, Phase 13 ships with a deliberate misbehaviour matrix — a
set of test plugins that crash on process, hang on load, return NaN, report
absurd latency, and fail state restore. **INCDAW must survive every one of them
with the project intact.** The FL Studio installation present on this machine
also provides real third-party plugins for integration testing.
