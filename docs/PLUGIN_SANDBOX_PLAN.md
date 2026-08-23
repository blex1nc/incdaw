# Out-of-process plugin hosting — the plan (C15)

Status: **plan only. Nothing here is implemented.**
Written: 2026-08-23, track C ("bridge").

---

## 1. The problem, stated exactly

A plugin runs in INCDAW's address space. A plugin that dereferences a null
pointer, writes past a buffer, or calls `exit()` therefore takes the
application with it — and with it the unsaved project, the take that was
being recorded, and the arrangement of the last two hours.

This is not a hypothetical. `tests/plugins/incdaw-testcrash.clap` exists
precisely because it was worth having a plugin that crashes on purpose,
and today the only thing protected from it is the *scanner*.

What already exists:

- `platform::ChildProcess` — fork/exec, stdout captured, and a `signalled`
  end state that distinguishes "it crashed" from "it declined".
- `PluginScan` runs discovery in that child, so a plugin that crashes while
  being *scanned* costs one scan.
- `PluginRegistry` blacklists what crashed, and `clearBlacklist` earns a
  retry.

What does not exist: any of that at **hosting** time. Once a plugin is
loaded into the graph it is inside the process for good.

## 2. What "sandboxing" will and will not mean here

**Will**: a crashing plugin loses that plugin — its slot goes silent, the
UI says so, the project keeps playing, and nothing is lost.

**Will not**: a security boundary. A plugin is code the user chose to
install and it runs with their privileges either way. The isolation is for
*stability*, and claiming more would be a claim the implementation does not
support. (macOS App Sandbox / entitlement-based isolation is a separate
question, tied to distribution, and out of scope here.)

## 3. Shape

```
  INCDAW process                     one host child per plugin
  ┌──────────────────┐               ┌────────────────────────┐
  │ RemotePluginNode │──shared mem──▶│ incdaw-pluginhost      │
  │  (engine::Node)  │◀──shared mem──│  loads the real plugin │
  │                  │               │  processes blocks      │
  │ RemotePluginProxy│──control fd──▶│                        │
  └──────────────────┘◀──control fd──└────────────────────────┘
```

- **`incdaw-pluginhost`** — a new binary beside `incdaw-pluginscan`, which
  is already built and already knows how to load a plugin.
- **Audio** crosses a shared-memory ring: one region per instance, laid out
  as `[header][input planes][output planes]`, sized at open time for
  `maxServiceableBlockSize`.
- **Events and parameters** cross the same region in a fixed-capacity
  message area, not a second channel — one region means one synchronisation
  problem.
- **Control** (open, close, save/restore state, editor) crosses a socket
  pair, where blocking is allowed because none of it is on the audio thread.

## 4. The hard part: the audio thread must not block

The realtime contract (docs/AUDIO_ENGINE.md §1) forbids blocking, so
`RemotePluginNode::process` cannot simply wait for the child.

The design that satisfies it:

1. The node writes the block into the ring and signals the child through a
   POSIX semaphore (`sem_post` is wait-free on macOS).
2. It then waits with a **bounded** timeout — a fraction of the block's own
   duration, e.g. 60% of `frames / sampleRate`.
3. On timeout it outputs silence for that block and counts a miss. The
   session continues; a plugin that is late is a plugin that glitches,
   not one that stops the transport.
4. Consecutive misses past a threshold retire the instance to the same
   "silent slot, reported in the UI" state a crash produces.

**This is the trade being made, and it should be named**: out-of-process
hosting costs a round trip per block and turns "the plugin is slow" from a
dropout into a glitch. In exchange, "the plugin is broken" stops being a
lost session. The user chooses per plugin (§7).

## 5. Detecting the crash

The child is `waitpid`-ed on a supervisor thread, never on the audio
thread. `ChildResult::End::signalled` is already the signal that means
"crashed" rather than "declined".

On a crash:

- the node's next `process` sees the instance marked dead and outputs
  silence (an already-bounded path — no new realtime work);
- the supervisor records it against the plugin in `PluginRegistry`, which
  already has a blacklist and already has a way to clear it;
- the UI marks the slot, with the plugin's name and "crashed"; the project
  is untouched, so a save afterwards is a project that still references the
  plugin and will try it again next time.

**Not** auto-restart on the first crash. A plugin that crashes on the third
note crashes again on the third note, and a host that keeps relaunching it
turns one glitch into a stutter. Restart is a button.

## 6. State, editors and latency

- **State** crosses the control channel as the same opaque blob the
  in-process path uses (docs/PLUGIN_HOST.md §6), so the project format does
  not change at all.
- **Editors** are the genuinely awkward part. A plugin's `NSView` belongs
  to the child's process, and cross-process view embedding on macOS means
  either `NSViewBridge`-style remote views or a child-owned window. **The
  first increment will not embed**: an out-of-process plugin's editor opens
  as its own window, owned by the child. That is a visible difference and
  it should be stated to the user rather than discovered.
- **Latency** is reported over the control channel at open and on change,
  and feeds the existing PDC path unchanged.

## 7. Rollout — this is a program, not a patch

| Phase | Deliverable | Verified by |
|---|---|---|
| S1 | `incdaw-pluginhost` binary: loads a plugin, reports parameters and latency, exits cleanly. No audio yet. | Existing scan tests extended |
| S2 | Shared-memory ring + control socket; a **loopback** host that copies input to output. | Bit-exactness against the in-process path |
| S3 | Real processing through the child; bounded wait, miss counting, silence on timeout. | The gain plugin's output matches in-process, sample for sample |
| S4 | Crash containment: `incdaw-testcrash.clap` hosted out of process, made to crash, session survives. | A test that would today take the suite down with it |
| S5 | Per-plugin opt-in in Settings, plus a global default. Editors in a child-owned window. | Manual, plus a settings round-trip test |
| S6 | Retire-on-repeat-crash, restart button, registry integration. | Fault-injection tests |

Each phase leaves the tree working: in-process hosting stays the default
throughout, and out-of-process is a per-plugin choice until it has earned
being the default.

## 8. Risks

- **Latency budget.** A round trip per block at 128 frames is 2.7 ms of
  wall clock to spend on scheduling. S3 measures it before S5 offers it.
- **Two code paths for hosting.** Mitigated by making `HostedPlugin` the
  seam: the remote instance implements the same interface, so nothing above
  it knows which side of the boundary the plugin is on.
- **Editor divergence** (§6) is a real UX regression for out-of-process
  plugins and the reason it is opt-in.
- **Shared memory lifetime.** A child killed mid-block leaves a region
  mapped; the supervisor unmaps and unlinks on reap, and the name carries
  the parent pid so an orphan cannot be reused.

## 9. What this plan deliberately does not do

- No security claim (§2).
- No process-per-*format* or process-pooling in the first program: one
  child per instance is more processes than strictly necessary, and it is
  also the only shape where one crash cannot take a second plugin with it.
  Pooling is an optimisation to make once the numbers from S3 exist.
