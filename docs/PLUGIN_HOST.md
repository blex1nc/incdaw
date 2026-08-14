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

- Parameters are discovered once and cached in the registry.
- Every parameter maps to INCDAW's **generic** automation subsystem — there is
  no plugin-specific automation code anywhere (CLAUDE.md §10).
- Parameter changes from the UI reach the audio thread through the same
  lock-free command queue as everything else.
- Plugin-originated parameter changes (a user turning a knob in the plugin's own
  editor) flow back and are recordable as automation.
- Sample-accurate parameter changes are honoured where the format supports them
  (CLAP natively; VST3 via sample-offset parameter queues).

---

## 6. State

The isolated `Plugin State System` node flagged by Graphify as under-specified
in the constitution is defined here.

Plugin state is an **opaque binary blob** owned by the plugin, stored per
instance in the project package under `plugins/` (docs/PROJECT_FORMAT.md).
INCDAW never interprets it.

Rules:
- State is saved on project save and on freeze/bounce.
- State survives a plugin crash: the last successfully-saved blob is retained.
- A missing plugin at load time does **not** discard its state — the node
  becomes a placeholder that retains the blob, so installing the plugin later
  restores the session intact.
- State is versioned alongside the project format so a future migration can
  reason about it.

---

## 7. Editor / UI bridge

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

- Each instance reports its latency in frames.
- Latency feeds PDC at graph-compile time (docs/AUDIO_ENGINE.md §7).
- A plugin that changes its latency while loaded triggers a background graph
  recompile and atomic swap.
- Plugins that report **wrong** latency are a known real-world problem; a manual
  per-instance latency offset is provided as an escape hatch.

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
