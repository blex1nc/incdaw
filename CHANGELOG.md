# INCDAW — Changelog

All notable changes to this project are recorded here.
Format loosely follows Keep a Changelog. The project is pre-release; there is no
public version yet.

---

## [Unreleased] — UI build-out

### UI — the Piano Roll gets its ruler, its ghosts and its key — 2026-08-23

**Added**

- **A numbered bar ruler across the top.** The Playlist and the Channel
  Rack both had one; the Piano Roll was the last pane with a time axis
  and no way to read it. Bars numbered, beats ticked, the playhead
  running through it. A click there is a click on a bar number, not a
  new note on whatever key sits under it.
- **Ghost notes** — what the pattern's other channels are playing, drawn
  faded behind this one's and never selectable. Writing a counter-line
  against a part you cannot see is guesswork. `G` toggles them.
- **Scale highlighting.** Rows outside the key signature are dimmed, so
  the notes that belong are the ones the eye lands on. The key and scale
  are the ones the nudge tool (`[` and `]`) already works in, so the grid
  and the tool cannot disagree about what "in key" means.
- **Key names on the keyboard** — a C every octave, which is what makes
  the pitch axis countable. Bar numbers and key names are the only two
  things in this pane that are not rectangles; they ride as a recycled
  pool of `CATextLayer`s over the Metal layer rather than through a
  glyph atlas, so the renderer still draws one primitive in one call.

**Changed**

- `PianoRollModel::Viewport::height` is the note grid's height alone now
  — ruler above, velocity lane below, neither counted in it. Everything
  that draws or hit-tests a note is unaffected; `keyToY` and `yToKey`
  offset by the band, and both hit tests refuse points inside it.
- `collectVisibleNotes` takes an `append` flag, which is how several
  channels' ghosts are gathered into one list without a buffer each.

**Files**

- `src/app/PianoRollModel.{h,cpp}`, `src/ui/macos/PianoRollView.mm`
- `tests/unit/PianoRollTests.cpp` — 7 new cases


### UI — the Channel Rack, shaped like a step sequencer — 2026-08-22

**Added**

- **Pan, at last.** `Channel::pan` has been in the model since Phase 4
  and applied by the graph compiler since Phase 10 — serialized,
  compiled and audible, with no command able to set it, because the row
  had nowhere to put a knob. `SetChannelPanCommand` is that command:
  bipolar, clamped, merging, so a drag is one undo entry.
- **Two knobs where there was one fader.** Volume and pan, both turned
  by dragging vertically (140 points end to end), both recentred by
  double-click — unity for volume, centre for pan. Two knobs fit where
  one fader did, and the width they save goes to the steps.
- **Every row says what makes its sound**, set right inside the channel
  button: the instrument's name, and the zone count when it is a
  sampler. The rack used to name every channel and tell you nothing
  about any of them.
- **Steps are grouped in fours**, with a real gap between groups. Beats
  are read spatially before they are read by colour, and the grouping
  survives a colour scheme that shading does not. The gap belongs to the
  bar rather than to the scroll position, so scrolling never moves where
  a bar appears to start.

**Changed**

- **The row is shaped like a step sequencer's**: mute lamp, solo, the
  channel's own button, volume, pan, then the steps — the order the
  hardware ones had, and the one that matches how a row is used.
- **The lamp is the mute switch and the colour swatch at once**, lit in
  the channel's colour when it is heard and dark when it is not. A
  separate swatch said the same thing twice and cost the row width.
- The channel button is the widest control in the row, tinted with the
  channel's colour, because it is the one that opens the instrument.
- `swatchRect` was the model's only geometry the view ignored and
  recomputed for itself. The two agree again.
- Step offsets come from one function (`stepOffset`) that the grid, the
  ruler and the hit test all measure with. The group gaps make the
  offset non-linear in the step index, and three separate calculations
  would have drifted apart.

**Files**

- `src/app/commands/ChannelCommands.{h,cpp}`, `src/app/ChannelRackModel.{h,cpp}`
- `src/ui/macos/ChannelRackView.mm`
- `tests/unit/ChannelRackTests.cpp` — 8 new cases

### UI — the Piano Roll gets a velocity lane — 2026-08-22

**Added**

- **The velocity lane** (FL calls it the event lane), under the grid,
  shown by default and toggled with `E`. One stem per note that starts
  in view, in the channel's colour, carrying the same
  velocity-to-brightness rule the notes do — so a bar and its note are
  recognisably the same object seen twice. Three faint lines at 32, 64
  and 96 give the scale, because the Metal renderer draws rectangles and
  cannot number an axis.
- **Velocity is editable at last.** `SetVelocityCommand` has existed and
  been tested since Phase 6; nothing in the UI could reach it, because
  the grid's vertical axis is already pitch. Click or drag a stem to set
  it. Dragging a stem that is part of the selection edits the whole
  selection — the rule move and resize already follow. The drag merges
  into one undo entry, and overshooting the lane pins at 1 and 127
  rather than wrapping. Velocity 0 stays unreachable: it is note-off.
- The whole column is the target, not just the filled part of a bar —
  otherwise the quietest notes would be the hardest ones to make louder.
  Where a chord puts several stems in one column, the topmost wins, the
  same rule `noteAtPoint` uses.

**Changed**

- Grid lines and the playhead now run through the lane: it shares the
  grid's time axis, and a beat that stopped at the boundary made the two
  read as unrelated panes.
- `PianoRollModel::Viewport::height` is now the note grid's height rather
  than the view's, with `velocityLaneHeight` beside it. The lane is taken
  out of the grid's share, never added to the view's, and the grid never
  drops below 140 points — in a short window the lane gives way instead.

**Files**

- `src/app/PianoRollModel.{h,cpp}`, `src/ui/macos/PianoRollView.mm`
- `tests/unit/PianoRollTests.cpp` — 11 new cases


### UI — the machine is configurable, the keyboard plays, and every command is one keystroke away — 2026-08-22

**Added**

- **Audio and MIDI Settings** (Cmd+,): output and input device, sample
  rate, block size, which MIDI sources to connect, and whether the input
  opens at launch. Applying reopens the device and the MIDI client in
  place — the playhead is preserved in TICKS, so a sample-rate change
  cannot move it — and the status line reports what the device actually
  GRANTED rather than what was asked for. A device that refuses to open
  falls back to the system default instead of leaving the app silent.
  Until this window existed, INCDAW played through whatever the system
  default happened to be and latency was not tunable at all.
- **MIDI hardware is connected.** `platform::MidiDevice` is opened at
  launch and on every apply, and its messages reach
  `engine::MidiInput`. This was the last missing link in a chain that has
  otherwise been complete and tested since Phase 5: a keyboard plugged
  into the Mac reached CoreMIDI and stopped there, which made live
  playing and MIDI learn dead ends in the running application. An empty
  source list means every source.
- **Command Search** (Cmd+K): type a few letters, run anything. The
  palette walks the menu bar — so an entry can never drift from the menu
  it mirrors, shortcut included — and lists the registry's actions and
  the live undo/redo beside them.
- **A registered action table**: `app::registerStandardActions` gives Add
  Channel, Add Pattern and Add Track ids that the palette, a shortcut, a
  controller mapping or a future script can all reach (CLAUDE.md §26).
  It lives in `app/`, not in the shell, so it is testable — the registry's
  table was empty in the running application before this.
- **The workspace is remembered**: window position and size, the active
  pane and pattern/song mode are restored at launch. A frame that no
  longer lands on an attached screen is centred rather than restored off
  the edge.

**Changed**

- **Undo and redo work from the Edit menu, and say what they will do**:
  "Undo Move Notes", "Redo Quantize Notes", from
  `CommandRegistry::undoName()`. They route through one shell path that
  resyncs every pane (docs/DECISIONS.md D-037), which also fixes the
  Piano Roll keeping selection indices into notes an undo taken in
  another pane had already removed. Cmd+Z is handed back to the field
  editor while a text field has focus.
- The status line advertises Cmd+K instead of Cmd+Z — undo now names
  itself in the menu — and reports a MIDI client that would not open.
- The window lays itself out from its real size rather than from the
  1280x800 default, so a restored window has no overhanging panes.

**Files**

- `src/app/AppSettings.{h,cpp}`, `src/app/StandardActions.{h,cpp}`
- `src/ui/macos/SettingsWindow.{h,mm}`, `src/ui/macos/CommandPalette.{h,mm}`
- `tests/unit/AppSettingsTests.cpp`, `tests/unit/StandardActionsTests.cpp`
- Decisions D-036 (settings are not project data) and D-037 (one undo caller)


### UI — the design language finished, and the readouts made live — 2026-08-20

**Added**

- **Metronome**: `MetronomeNode` — which has existed since Phase 3 and is
  the instrument the transport's timing is measured with — is compiled
  into the graph when the toggle is on, mixed into the master so it can
  never reach a render or a stem. Control-bar button, Audio menu item
  (Shift+Cmd+M), and a checkmark that follows the state.
- **Tempo is editable**: drag the readout (a quarter BPM per point,
  Option for hundredths), double-click to type, or Shift+Cmd+T to tap it
  in. `SetTempoCommand` merges, so a drag is one undo entry; the project
  is the source of truth, the transport takes the same map, and the
  graph rebuilds.
- **Time signature is real**: the display no longer says 4/4 regardless.
  It reads the project's signature — which the tempo map and the file
  format have both carried since the beginning — and clicking it offers
  the signatures a session uses. `SetTimeSignatureCommand` is undoable,
  and the position readout counts bars in the signature it is given.

- **The Channel Rack has a step ruler**: beats numbered over the grid,
  a tick between them, the playhead's step lit, and the rows pushed
  down by exactly its height. A click in the band is not a step.
- **The Mixer shows its insert chain in the strip**: four slots between
  the pan and the fader, in signal order, each with a lamp that
  switches the slot in and out and a name that opens what it has to
  edit. An empty slot offers the catalogues; a longer chain says how
  many did not fit. The context menu still reaches every slot.

**Changed**

- A compiled project graph now owns **its own copy of the tempo map**.
  The nodes that convert ticks to frames hold a pointer to it for as
  long as they render, so a map shared with the transport could not be
  rewritten while the audio thread was inside it. A tempo edit is a
  rebuild: the new graph carries the new map, the old one keeps the map
  it was compiled against until it is retired.
- `MetronomeNode` takes a tempo map rather than the transport, and reads
  the transport's state from the block like every other node.
- The Browser, the insert parameter panel and the spectrum analyzer —
  written on a parallel line before the theme existed — draw through it
  now: the Browser has the pane header, ground and accent selection the
  other panes have, and no view in the shell declares a colour of its
  own any more (D-035).

### Fixed — the hum at idle — 2026-08-20

INCDAW made a sound while it was doing nothing. Opening the application
was enough: the default project's first note sits on tick 0, the
playhead parks on tick 0, and every block re-triggered it.

A stopped transport still renders — a synth's release and a delay's
tail have to keep coming out, and input monitoring has to keep working
— so `Transport::processBlock` keeps handing the graph one segment
while stopped. That segment carries the SAME position every block, and
the nodes that read the timeline had no way to know: `InstrumentNode`
re-collected the note under the playhead ~94 times a second (and hard
-killed the voice in between, from the discontinuity check that a
parked position looks like), and `AudioClipNode` replayed the few
hundred frames the playhead sat on, forever. What the ear got was a
hum at the block rate.

`ProcessContext::playing` now carries the transport's state to every
node, `CompiledGraph::process` takes it, and the engine passes
`transport_.isPlaying()`:

- The instrument's sequence and audio clips render nothing while the
  timeline is parked.
- The instrument ends its sequenced notes once, on the first stopped
  block — not every block, which also means a note played on a
  keyboard into a parked project now sustains instead of being cut
  down at the next block boundary.
- Automation keeps evaluating: a parked playhead's parameter values
  are exactly what the mixer should show.
- Offline rendering is untouched — it never stops, and the flag
  defaults to true.

Three regression tests in `tests/unit/StoppedTransportTests.cpp`, each
verified to fail against the old behaviour. docs/AUDIO_ENGINE.md §5.

### UI — one design language for the shell — 2026-08-17

The window is now drawn through a single visual vocabulary instead of
per-pane greys: FL Studio's density (dark ground, beat-grouped step pads,
colour as the primary identifier, always-visible mode and load) inside
GarageBand's calm (rounded geometry, soft gradients, round transport
buttons around a centre display, named rounded regions). Nothing is
copied from either product — every shape is drawn from primitives and
the palette is INCDAW's own (docs/DECISIONS.md D-035).

**Added**

- `ui/macos/Theme.{h,mm}`: the palette (`Ink` roles), metrics, type and
  the drawing vocabulary — panel, well, step pad, region, transport
  button, knob, slider, fader, meter, toggle, LCD, tab, playhead.
- `ui/macos/ControlBarView.{h,mm}`: the control bar (rewind / stop /
  play / record / loop, pattern-song switch, centre display showing
  bar.beat.tick, tempo, mode and material, editor tabs, CPU and output
  meters) and the hint bar along the bottom edge.
- Corner radii in the Piano Roll's Metal renderer: `ui::Rect::radius`,
  rounded by a signed-distance field in the fragment shader — still one
  instance per rectangle, still one draw call.
- Pattern clips in the Playlist draw a preview of the notes they will
  play, so two pattern clips no longer look identical.
- Rotating default colours for new channels, patterns, tracks and mixer
  nodes (`project::Project`), with a regression test.
- A dB readout under every mixer fader.

**Changed**

- Every pane (Channel Rack, Pattern list, Piano Roll, Playlist, Mixer,
  Audio Editor) now draws through the theme and owns no colours.
- The window, its sheets and hosted plugin editors declare the dark
  appearance, so AppKit's own controls match the surfaces around them.
- The Metal layer declares sRGB; the Piano Roll no longer drifts in
  colour away from the AppKit panes beside it.
- Channel Rack geometry: taller rows, square mute/solo switches, wider
  pads; the transport's two `NSSegmentedControl`s and the status
  `NSTextField` are gone, replaced by the control bar and hint bar.

**Not changed**

- No engine, project-format, command or audio behaviour. Tempo remains
  read-only in the display: editing it needs an undoable command, which
  is a feature, not a repaint.

The twenty engineering phases produced a complete core; what follows is
the UI build-out — FL-Studio-class workspaces over that core, in
increments. Each increment ships working, tested behaviour.

### UI build-out, increments 5–10 — the recorded gap list, closed — 2026-08-16

One session over the standing polish/gap list. Every item that needed
neither a new dependency (§41 approval) nor hardware landed:

**Added**

- **Insert reordering** (`MoveInsertCommand`, mixer menu Move Up/Down):
  chain order is signal order, no longer only add order.
- **Live parameter panels**: open panels follow live values at 5 Hz
  (builtin state decode, hosted `get_value`, instrument model), so
  automation, MIDI knobs and undo are visible; drags are never fought.
- **clap_host_latency.changed**: the CLAP host implements the latency
  extension; changed lands on an atomic flag, housekeeping re-reads
  under the creation-time hostile cap and recompiles. Proven end to end
  against the latency test plugin. `params->flush()`-while-idle recorded
  as not currently applicable (no code path can queue into an instance
  whose process is not running) in PLUGIN_HOST §5.
- **Export progress and cancel**: RenderOptions.progress (coarse, false
  cancels); the shell renders on a background thread over a copy of the
  project behind a modal progress window, polling shared atomics.
- **Touch and latch automation** (the 11b deferral): write spans the
  pass, touch closes a segment per drag (mixer mouse-up reports gesture
  ends) so existing envelope survives between drags, latch holds the
  last value to the stop tick. Menu: Record Automation > Off/Write/
  Touch/Latch; switching modes lands the pass first.
- **Audio editor Cut/Copy/Paste/Delete**: three pure verbs
  (extractRegion/deleteRegion/insertAudio — mismatched rate or channels
  refused, never resampled silently) and two commands with bit-exact
  undo; paste carries its piece so redo replays recorded bytes; copy is
  deliberately not a command. App-local audio clipboard.
- **Limiter (Lookahead)**: a new catalogued effect — fixed 2 ms window
  (a parameter would leave PDC stale between rebuilds), sliding-maximum
  gain so attenuation lands before the transient, latency reported into
  the graph's existing compensation through the hosted plugins' door.
  The classic limiter is untouched; Phase 15's node-level null holds.
  The catalogue factory now takes the sample rate.
- **Spectrum analyzer**: `engine/dsp/Fft` (own radix-2, allocation-free,
  proven against a naive DFT), the Analyzer accumulating a Hann-windowed
  mono downmix and publishing dBFS bins seqlock-style; "Open Editor" on
  an analyzer opens a log-frequency spectrum view. The compiled graph
  exposes insert nodes by slot for effect-specific surfaces.

**Remaining, with their gates** (recorded in HANDOFF): AU then VST3 and
FLAC/MP3 need dependency approval (§41); MIDI output/clock/sync needs a
platform MIDI-out subsystem and hardware verification; sandboxed plugin
processing is its own architectural program; graphical zone view,
per-zone envelopes/filters/LFOs, and plugin out_events → recordable
automation are future increments.

### UI build-out, increment 4 — mapping, zone and instrument editors — 2026-08-16

The last of the recorded engine-side gaps a UI pass surfaces: instruments
became editable and persistent, samplers became mappable, and MIDI
mappings became visible.

**Added**

- Format 1.5 (additive, migration + frozen fixture): channels carry
  `instrumentParameters[]` — stored plain values the compiler applies
  through the instrument's ParameterSink after construction (D-034). The
  model is the source of truth at every rebuild, exactly like mixer
  volume, so the values survive rebuilds AND save/load — the persistence
  the increment-2 deferral was waiting for.
- `SetInstrumentParameterCommand` (mergeable — a slider drag is one undo
  entry; undoing a never-touched parameter removes the entry rather than
  storing the default). Channel rack context menu gained
  "Edit Instrument…": the increment-2 panel reused verbatim over the
  builtin instrument catalogue (sampler filter/LFO/ADSR, synth ADSR/gain
  — the Phase 14 "engine + setters only" gap closed).
- "Sampler Zones…" (rack context menu): the zone editor — one row per
  zone with root key, key range, velocity range, gain, reverse and
  Remove; "Add Sample Layer…" appends zones for multisampling
  (`AddSamplerZoneCommand`, sharing LoadSampleCommand's asset rule via
  one extracted minting helper — same file, same asset, never
  duplicated). Every edit is a mergeable, undoable command
  (`SetSamplerZoneCommand`/`RemoveSamplerZoneCommand`) and rebuilds the
  graph immediately.
- Audio > MIDI Mappings…: every mapping listed as
  "CC n (channel) → parameter · target name", removable undoably; the
  list refreshes on every rebuild while open, so learn and forget from
  the mixer menu stay in sync with it.

**Known limitation (recorded)**

- The zone editor edits numbers, not pictures — a graphical key-range
  layout view is future polish. Zone start/end/loop points are editable
  only in the model (fields deliberately left off the first row layout).
- Hosted instruments (when they arrive) will persist through
  `instrumentStateFile` blobs, not model values; the panel covers
  builtins only and beeps for anything else.

### UI build-out, increment 3 — export options dialog — 2026-08-16

File > Export Audio… stops hardcoding master/float32/WAV/engine-rate.

**Added**

- An options dialog fronting everything `project::OfflineRender` already
  implemented: target (master, stems — one file per non-master mixer
  track, tracks — one file per channel), WAV or AIFF, 16/24-bit PCM or
  32-bit float, target sample rate (44.1/48/88.2/96 k through the
  windowed-sinc resampler), tail seconds, normalize, TPDF dither, and an
  "only the loop range" region export taken from the transport's loop.
- Stems and tracks export into a chosen directory, one file per source,
  named by the mixer track or channel with path-hostile characters
  replaced and case-insensitive collisions suffixed
  (`app::session::exportFileName`, tested). The first failing file stops
  the run with its name; finished files stay.

**Known limitation (recorded)**

- The render loop runs synchronously on the main thread — no progress
  bar, no cancel. Fine at ~70x realtime; a progress sheet is recorded
  polish for a later increment.

### UI build-out, increment 2 — generic insert parameter panel — 2026-08-16

The ten builtin effects finally have a UI, and "Open Editor" never dead-ends.

**Added**

- `ui/macos/InsertParameterPanel`: every parameter of an insert slot as a
  labelled slider (stepped parameters snap to ticks). The panel holds row
  data and a write block, never an engine pointer — sinks die with their
  graph, so each slider move re-resolves the slot against the live graph.
- "Open Editor" on a mixer insert now opens the plugin's own GUI when it
  has one, and the generic panel otherwise (builtin effects, hosted
  plugins without editors, bypassed slots). Panel writes mark the
  document dirty and persist through the existing state capture at save.
- `CompiledProjectGraph::insertSinkFor(slot)`: the compiler now exports
  the same per-slot parameter sinks its own automation and MIDI bindings
  resolve against.
- `BuiltinEffect::decodeState`: the id-keyed blob decoder factored out of
  `loadState` and shared with the panel's value display — one decoder,
  no second opinion about the layout.
- `ClapInstance::readParameter` (CLAP `params.get_value`, main-thread):
  hosted plugins report their current values to the panel; defaults fill
  in for plugins that cannot answer.

**Fixed**

- Builtin insert state now survives graph rebuilds. Every compile
  recreates builtin nodes at their defaults, so any value set on the live
  node (a MIDI-mapped knob, and now the panel) silently reset on the next
  note edit. `captureBuiltinInsertState`/`restoreBuiltinInsertState`
  carry the id-keyed blobs across the swap in memory; hosted instances
  already persisted (D-031) and are deliberately not touched. Open/New
  clear the live handle table first, so carry-over can never leak values
  across projects whose entity ids collide.

**Known limitation (recorded)**

- Panel writes are not undoable (insert parameter values live in engine
  state, not the model — same as turning the knob in a plugin's own GUI).
- An open panel does not live-refresh while automation moves the same
  parameters; it shows values as of opening.

---

### UI build-out, increment 1 — project lifecycle safety — 2026-08-16

Unsaved work is no longer lost silently. The gaps this closes were the
standing ones recorded at 0.9.0: dirty-prompt on quit, autosave,
recent projects.

**Added**

- Dirty tracking: any command-based mutation (undo and redo included,
  via the undo-depth watch in housekeeping) and the one command-less
  mutation (MIDI import) mark the document edited; the window close
  button shows the standard dot. Save, Open, and New clear it.
- Quit guard: `applicationShouldTerminate` asks Save / Cancel /
  Don't Save when the project is dirty. Open and New ask through the
  same guard before discarding the current project.
- File > New (⌘N): resets to the same starter document launch seeds,
  through the same adoption path Open uses.
- Autosave, every two minutes while dirty: a saved project autosaves
  beside itself as `<name>.autosave.incdaw`; an unsaved one autosaves
  to Application Support. Autosave never touches the dirty flag, the
  title, the recents, or the user's file, and failures log rather than
  alert. Opening a project whose autosave is newer offers the autosave
  (⌘S still targets the real project, and the document opens dirty). A
  leftover unsaved-project autosave — only a crash leaves one, a normal
  quit deletes it — is offered for recovery at the next launch.
- File > Open Recent: the last ten saved or opened projects in user
  defaults, with Clear Menu; entries that no longer load are pruned.
- `app/ProjectSession.{h,cpp}`: the headless decisions (recents-list
  update, autosave destination, autosave-newer comparison), tested in
  `tests/unit/ProjectSessionTests.cpp` — the same headless-model split
  the view models use.

**Known limitation (recorded)**

- A parameter changed only inside a hosted plugin's own editor window
  does not mark the document dirty; the shell cannot see it until the
  host-callback work (`clap_host_state.mark_dirty`, PLUGIN_HOST §6)
  lands.

### FL Studio 2026 feature parity, wave 1 — 2026-08-16

Eight feature blocks closing gaps against the FL Studio 2026 functional
reference (docs/FL2026_GAP.md holds the full analysis and status table).

**Added**

- **Piano Roll chord toolkit** (`app::music`, `ChordCommands`): chord
  detection with inversion naming (Chord Panel equivalent), Chord Stamp
  with bottom-up/top-down voicings, movement-minimising voice-leading,
  diatonic progression nudge. Wired: Option-click stamps, `1`–`8` pick
  the shape, `V` flips voicing, `C` names the selection, `[`/`]` nudge.
- **Note tools** (`NoteToolCommands`): strum with anchored ends,
  arpeggiate (up/down/up-down), legato to the next start, note labels
  (F2) on the per-note property that has serialized since Phase 5.
- **Clip splitting and timeline markers** (format v1.5): frame-exact
  audio splits with advancing source offsets, tick-exact pattern and
  automation splits, compile-equivalence asserted; `TimelineMarker`
  (region = marker with length) with add/edit/remove commands, ruler
  drawing, Option-click slice and `M`/Shift+`M` gestures.
- **Sidechain routing**: sidechain edges now compile into detector-only
  key inputs on the destination's compressor inserts, PDC-aligned,
  never summed into audio; "Sidechain Into" in the mixer strip menu.
- **EBU R128 loudness** (`incdaw.loudness`): BS.1770-4 K-weighting
  designed per sample rate, momentary/short-term/gated integrated via a
  constant-memory histogram, calibrated to the −3.01 LUFS reference.
- **Stereo separation** on every strip (mid/side, −1 mono … +1 wide,
  click-free ramp, automation key `stereoSeparation`) and **true
  pre-fader sends** through a unity tap spliced ahead of the fader.
- **Modulation effects**: chorus, flanger, phaser — bit-exact null at
  defaults; **Transient Split** (Transmitter equivalent): fast/slow
  envelope race, both/transients/sustain outputs, exact reassembly.
- **Time-stretch subsystem** (`engine::dsp::timeStretch`): offline
  WSOLA with transient locking (clicks survive both directions exactly
  once), stereo-coherent alignment, pitch shift via windowed-sinc;
  editor verbs Time Stretch…/Pitch Shift… with bit-exact undo of the
  file; Option-drag stretch at the clip edge (`StretchClipsCommand`);
  warped clips render at compile time and play through the graph.
- **Slicer**: `engine::audio::detectOnsets` plus `SliceAssetCommand` —
  one undo lands a sampler channel (a zone per slice, chromatic keys)
  and the pattern replaying the loop's timing; "Slice to New Channel"
  in the editor's Audio menu.
- **Browser** (`app::Browser` + `INCDAWBrowserView`): the pane INCDAW
  never had. Roots (the user's own folders on a first launch), listings
  that put folders first and hide dot-files, recursive name search
  capped at 500 results and 8 levels, favourites and recents — all
  persisted beside the plugin catalogue, never inside a project.
  Project packages and plugin bundles are named as themselves and never
  descended into. Leftmost in the workspace, hidden and shown from
  View > Browser (⌘B); a double-click opens a project or imports a MIDI
  file.
- **Preview** (`engine::AuditionPlayer`): a click in the Browser auditions
  the sample, mixed by the engine after the project graph — it sounds with
  the transport stopped and never costs a graph rebuild. The audio thread
  reads a raw pointer; the decoded buffer is released only once the block
  counter has passed the swap, the same grace a retired graph gets.
  Cross-rate files are repitched by interpolated reading; NaN containment
  now covers the preview too.
- **Drag into the project**: browser rows are drag sources, and the
  Channel Rack and Playlist accept file drops (from Finder as well). A
  sample dropped on a channel replaces its sound, on empty rack space
  becomes a sampler channel, on a playlist lane becomes an audio clip at
  the snapped drop tick, as long as the file. Each is one undo with ids
  that survive redo; asset import is now one shared helper
  (`app::AudioAssetImport`) rather than a copy per command.

- **Audio Unit hosting** (`platform::AudioUnitHost`, `plugins::AudioUnitInstance`):
  macOS's own plugin format, hosted as effects. Components are enumerated
  from the system registry — no scan, no plugin code run — and appear in
  the mixer's Add Insert menu beside the scanned CLAP plugins. Stereo
  float in-place rendering, parameters, reported latency into the existing
  PDC, ClassInfo state through the same StateIO seam project files
  already use, and the format's generic editor view.
- **`plugins::HostedPlugin`**: the interface the second format forced.
  PluginNode, the instance manager, delay compensation, plugin state
  files and the shell's editor windows are written against it; CLAP and
  AU are two implementations of it and nothing above the interface knows
  which is which.

**Tests**

- 483 → 580 cases; every feature carries its own quality gates
  (calibrated LUFS, analytic sidechain levels, WSOLA click preservation,
  chord-theory pins, format round-trips and the v1.5 fixture).

---

## [0.9.0] — 2026-08-16 — the core is complete

Every engineering phase of the roadmap (0–20) is done: the first
versioned release of INCDAW's core. Release notes and the install/update
procedure live in docs/RELEASE.md.

### Phase 20 — Release engineering — PHASE 20 COMPLETE — 2026-08-16

**Added**

- Version 0.9.0, stamped once in CMake and mirrored in `app/Version` —
  the binary, the bundle and the DMG name cannot disagree.
- docs/RELEASE.md: what a release is, how to cut one, the documented
  first-launch procedure on another Mac (ad-hoc signature vs Gatekeeper,
  D-009), the update process and the project-format compatibility rules
  it leans on, and the 0.9.0 release notes.
- `dist/INCDAW-0.9.0.dmg` builds, signs ad hoc, verifies its signature
  and its checksum, and carries FIRST LAUNCH.txt — the exit criterion's
  documented procedure. (Installing on a second Mac is the one step a
  test suite cannot perform; the procedure it would follow is written
  and verified to the edge of this machine.)

### Phase 19 — QA — PHASE 19 COMPLETE — 2026-08-16

**Added**

- Deterministic fuzzing (`FuzzTests.cpp`) of every reader that touches
  outside bytes: seeded random corruption and systematic truncation of
  project.json, manifest.json, pattern files, WAV and Standard MIDI
  Files. The contract under fuzz: error or succeed, never crash. Fixed
  seeds make any future failure an instant permanent regression test.
- Stress (`StressTests.cpp`): a 96-channel / 24-pattern / ~12k-note /
  192-clip project survives save → load (identity), compiles with every
  channel present, and renders finite, non-silent audio; and a
  long-session loop of 400 edit-rebuild-process cycles (undo-shaped
  edits included) stays finite and lands on the exact expected note
  count.
- The plugin crash matrix stands from Phase 13: a crashing plugin kills
  the out-of-process scanner and lands on the blacklist with its
  reason; the host survives. In-process processing crashes remain
  unsurvivable BY DESIGN until sandboxed hosting exists — recorded, not
  hidden.

**Exit criterion (docs/ROADMAP.md Phase 19) — MET**

- The whole suite is green — 483 cases, 1,335,729 assertions, Debug and
  Release — and no crash-class defects are open: the fuzzers found none
  to file.

### Phase 18 — Performance — PHASE 18 COMPLETE — 2026-08-16

**Added**

- `incdaw-bench` (tools/bench): the reproducible measurement harness the
  phase is built on — graph rebuild latency, offline render throughput,
  per-effect block cost, resampler throughput. Baseline recorded in
  docs/PERFORMANCE.md §7.

**Measured, then changed**

- Resampler: the windowed-sinc kernel evaluated two transcendentals per
  tap per output frame. Now precomputed into a 512-phase × 64-tap table
  with linear phase interpolation. **547.9 ms → 18.6 ms for 10 s stereo
  (18.3× → 537.2× realtime, 29× faster)**; the −60 dB quality regression
  test passes unchanged.
- The bench itself was measuring the EQ's *bypass* (midpoint parameters
  put its gains at 0 dB, which is a skipped band). Parameters now sit at
  75 % of range; the honest EQ number is 15.6 ns/frame — no optimisation
  warranted.

**Measured, deliberately left alone (CLAUDE.md §27)**

- Rebuild latency: 0.20 ms at 64 sampler channels + master chain.
- Offline render: ~70× realtime at 64 channels.
- Every builtin effect: < 0.1 % of the 512-frame block budget.

**Exit criterion (docs/ROADMAP.md Phase 18) — MET**

- Every optimisation has a documented before/after measurement
  (docs/PERFORMANCE.md §7); everything not optimised has the measurement
  that justified leaving it alone.

### Phase 17 — Rendering and export — PHASE 17 COMPLETE — 2026-08-16

**Added**

- `project::renderProject` / `renderProjectToFile`: offline rendering with
  NO separate offline DSP — the same compiler, the same graph, the same
  block loop the audio callback runs, captured instead of played. Assets
  always preload (never stream) offline, because determinism is the
  contract. Master mix, stems (solo a mixer node), individual tracks
  (solo a channel), regions, tail seconds, normalization to exactly 1.0,
  deterministic TPDF dither at 16-bit, pcm16/24/float32.
- `engine::dsp::resample`: offline windowed-sinc sample-rate conversion
  (Blackman-Harris, 32 taps/side, correct anti-alias cutoff when
  downsampling). Measured < −60 dB RMS error on a 1 kHz tone 48 → 44.1.
- `engine::AiffFile`: big-endian PCM 16/24 AIFF writer (80-bit extended
  rate and all).
- `engine::SmfFile` + `project::exportArrangement` / `importAsPattern`:
  Standard MIDI File format-1 write and read (running status, foreign
  PPQN rescale, orphan note-off handling); export flattens the
  arrangement through the same pattern compiler playback uses; import
  lands as an editable pattern with a channel per track.
- File menu: Export Audio… (WAV/AIFF), Export MIDI…, Import MIDI…

**Exit criterion (docs/ROADMAP.md Phase 17) — MET and measured**

- `RenderTests.cpp`: the offline render equals a captured realtime-style
  drive of the same compiled graph EXACTLY — float-for-float equality on
  every sample of both channels, the criterion's "byte-identical".

**Deferred, recorded**

- FLAC needs libFLAC (or an encoder of our own) — a dependency decision
  the constitution reserves for the user (§41). MP3 likewise. CC/pitch
  bend lanes in SMF import; AIFF reading.

**Tests**

- 478 cases, 1,136,905 assertions, green in Debug and Release.

### Phase 16 — MIDI hardware and controller linking — PHASE 16 COMPLETE — 2026-08-16

**Added**

- `MidiMapping` in the model (project format **1.4**): a hardware control
  bound to a parameter exactly the way an automation lane is — registry
  key + target entity — plus a normalised, invertible output range.
  Frozen v1.4 fixture; 1.3 declares its migration path.
- `engine::MidiMapNode`: the hardware counterpart of the AutomationNode.
  Mappings resolve at compile time through the SAME `resolveApplier` the
  automation lanes use (the resolution was factored out, not duplicated),
  ride the same graph, and write the same smoothed setters — a knob and a
  lane cannot disagree about a parameter.
- Instruments joined the parameter system: `Instrument::parameterSink`,
  implemented by the Sampler (ADSR, filter, LFO — ids in `SamplerParam`)
  and the reference synth; `InstrumentNode` surfaces it, and a lane or
  mapping whose TARGET is a channel resolves to that channel's instrument.
  `registerBuiltinInstruments()` mirrors the effects registration.
- MIDI learn: the mixer context menu arms "MIDI Learn Volume/Pan"; the
  MIDI input publishes its last CC through a packed atomic tap; the
  shell's housekeeping completes the gesture as an undoable
  `AddMidiMappingCommand` (re-learning a CC replaces its old binding,
  undoably). "Forget MIDI Mappings" removes a node's bindings.

**Exit criterion (docs/ROADMAP.md Phase 16) — MET and measured**

- `MidiMappingTests.cpp`: three mapped CCs drive a mixer volume, a sampler
  cutoff (instrument parameter through the channel target) and an insert's
  gain (plugin parameter through the identical SinkApplier machinery
  hosted plugins use) in one compiled graph's live MIDI — then the project
  is saved, reloaded, recompiled, and the same knobs still work.

**Deferred, recorded**

- MIDI clock/sync and hardware feedback (LED echo) — need MIDI OUTPUT
  device support in platform/, which does not exist yet. Listed in
  HANDOFF as Phase 16 leftovers, non-blocking.

**Tests**

- 466 cases, 286,224 assertions, green in Debug and Release.

### Phase 15 — The builtin DSP suite — PHASE 15 COMPLETE — 2026-08-16

**Added**

- `engine/dsp/effects/`: ten builtin effects on one shared base
  (`BuiltinEffect` = Node + ParameterSink + StateIO): Utility (gain, pan,
  width, polarity, mono), Filter (SVF LP/HP/BP), EQ 3-Band (RBJ shelves +
  peak), Saturator (tanh, dry/wet), Compressor (linked peak detector,
  log-domain gain computer, GR meter), Limiter (zero-lookahead, exact
  ceiling), Gate (attack/hold/release), Delay (feedback line, dry/wet),
  Reverb (Schroeder: 4 damped combs + 2 allpasses, stereo spread),
  Analyzer (bit-exact pass-through publishing peak/RMS).
- One catalogue (`BuiltinEffects.cpp`) drives everything: the compiler
  (a builtin slot is constructed exactly where a hosted plugin's factory
  would run — D-033), the automation registry
  (`ParameterRegistry::registerBuiltinEffects`, same key scheme and
  applier as scanned plugins), the mixer's new "Add Built-in Effect"
  menu, and slot naming.
- Builtin parameters persist through the existing plugins/<slot>.state
  path via a versioned, id-keyed blob; unknown ids are skipped on load.

**Exit criterion (docs/ROADMAP.md Phase 15) — MET and measured**

- Every effect passes a null test at its transparent settings (bit-exact,
  not approximate) AND matches an independent straight-line reference
  implementation of its own formula in `BuiltinEffectTests.cpp`. The
  no-special-casing half is proven in `BuiltinInsertTests.cpp`: a builtin
  slot compiles with no plugin host, carries StateIO, registers every
  parameter, and an unknown uid degrades to a warning and pass-through.
- The reference test caught and fixed a real limiter defect: release
  recovery applied after the ceiling check let a recovering gain push a
  sample ~0.004 dB over the ceiling; recovery now precedes the clamp and
  the ceiling holds exactly.

**Tests**

- 461 cases, 286,163 assertions, green in Debug and Release.

### Phase 14 (parts 4–6) — Loading, filter/LFO, disk streaming — PHASE 14 COMPLETE — 2026-08-16

**Added**

- `LoadSampleCommand` and the Channel Rack's "Load Sample…" context menu
  item: one undoable gesture ensures an AudioAsset (probing the header for
  metadata), gives the channel the builtin sampler identity and writes a
  full-range zone rooted at middle C. Undo removes the asset only if the
  command created it; a file already in the project is shared, not
  duplicated, and redo replays the same asset id.
- Per-voice state-variable filter (off/lowpass/highpass/bandpass, cutoff,
  resonance) and one retriggered sine LFO with depth-controlled pitch and
  cutoff destinations — realtime-safe atomic setters, per-voice filter
  state, block-rate coefficient unless cutoff is modulated.
- **Streamed sampler zones** (`engine::SamplerZoneStream`): the classic
  head-plus-pool design — the first `samplerHeadFrames` (~1.4 s) live
  decoded in RAM so a note starts instantly; past the head, a voice reads
  from one of 4 pooled `AudioStream`s whose window was steered to the
  hand-over point at note-on. Slots are claimed wait-free; more held notes
  than slots degrade to head-only voices rather than blocking or
  allocating. Forward, unlooped zones stream; looped and reversed zones
  preload whole (a loop must be resident to be seamless). The compiler
  applies the same size threshold as clips.

**Exit criterion (docs/ROADMAP.md Phase 14) — MET and measured**

- `SamplerStreamingTests.cpp` holds a chord across the keyboard (−12, root,
  +12) split over two velocity layers, streaming ~3 s from disk: zero
  underruns counted by the streams themselves, and the late-window RMS sits
  at the exact expected mix of the correct layers' constants.

**Tests**

- 445 cases, 177,464 assertions, green in Debug and Release. New coverage:
  load command (undo/redo/shared-asset/refusal), filter attenuation and
  passband, LFO depth-zero bit-identity, extreme-resonance finiteness,
  instant start from the head, pool exhaustion degradation and slot
  reclamation, compiler-level end-to-end streaming at the exact expected
  level.

### Phase 14 (part 3) — The sampler reaches the model — 2026-08-16

**Added**

- `Channel::samplerZones` (`ChannelSamplerZone`): the sampler's program in
  the MODEL — each zone names an audio asset by `EntityId` plus the mapping
  numbers, so the relinker can see a sampler's samples. Serialized
  additively as project format **1.3** (migration path declared; frozen
  hand-written fixtures for v1.2 *and* v1.3 added, closing the standing
  "no fixture" gap for 1.2).
- `plugins::Format::builtin` with `builtinSampler()` /
  `builtinSimpleSynth()` (`builtin:incdaw.sampler`,
  `builtin:incdaw.simplesynth`): builtin instruments share the plugin
  identity scheme instead of inventing a second one. An empty identifier
  still means "no instrument chosen" and still yields the default synth.
- `engine::SampleCache` (D-032): decoded audio cached by (path, size,
  mtime), shared immutably across graph rebuilds. Owned by the app, passed
  via `GraphCompileOptions::sampleCache`; zones AND preloaded audio clips
  resolve through it, so neither re-decodes per rebuild.
- The project compiler builds builtin instruments itself — the factory
  remains the seam for hosted formats (D-028). A sampler channel comes out
  of `compileProjectGraph` playing: zones resolved, cross-rate zones
  allowed (the sampler repitches by rate; the clip path still refuses
  cross-rate assets), a missing sample degrading to a warning and a silent
  channel that stays in the graph.

**Tests**

- 11 new cases (435 total, 170,219 assertions, green in Debug and
  Release): cache identity/invalidation/miss-not-cached; builtin
  identifier round-trips; v1.2 and v1.3 fixtures load; sampler channel
  renders audibly through the compiler, with and without the cache; the
  degradation paths (missing file, unknown asset id, unknown builtin uid).

### Phase 14 (part 2) — Sustain loops and the crossfaded seam — 2026-08-15

**Added**

- `SamplerZone` gained `loopStart`/`loopEnd`/`loopCrossfade`. A held note
  cycles the loop forever — through release too, which is what a sustain
  loop means; the envelope ends the voice, not the loop. The crossfade
  blends the loop's last N frames toward the material just before
  loopStart — the exact content the wrap lands on, so the junction is
  continuous by construction. A loop that does not fit its slice is ignored
  rather than repaired into something the user did not draw; reverse zones
  do not loop yet.

**Tests**

- 3 new cases (424 total, 170,135 assertions, green in Debug and Release):
  the loop cycles across thousands of frames and survives into release;
  the seam blends with exact midpoints and lands continuously; an unfit
  loop plays as none.

### Phase 14 (part 1) — The sampler core — 2026-08-15

**Added**

- `engine/instrument/Sampler` — zones in, sample-accurate voices out. A
  `SamplerZone` maps a shared, immutable decoded sample to key and velocity
  ranges with a root key, a start/end slice, reverse, and gain; a program is
  nothing but zones, and key/velocity LAYERING falls out of several zones
  matching one note. Playback repitches by rate through linear interpolation
  (a sampler is not time stretching, §16); a source at another sample rate
  is resampled by the same mechanism, so pitch stays true. ADSR envelope and
  voice stealing mirror the reference synth; 64 voices.
- `setZones` follows the structural-edit contract: build time only, like an
  insert edit — a zone edit is a graph rebuild. Live control is the
  realtime-safe envelope setters.

**Not yet (recorded, coming in later parts)**

- Loop points and crossfade, filters, LFOs, per-zone envelopes, disk
  streaming for long samples, the channel/model wiring and UI.

**Tests**

- 10 new cases (421 total, 170,123 assertions, green in Debug and Release):
  verbatim playback at root; octave-up reads twice as fast; key/velocity
  gating; layer selection and overlapping-zone summing; velocity scaling;
  reverse; slice bounds ending the voice; release ramp and allNotesOff;
  stereo/mono channel mapping; a 24 kHz source resampled at 48 kHz.

### Phase 13 (part 10) — The editor bridge — 2026-08-15

**Added**

- `ClapInstance` bridges CLAP_EXT_GUI for embedded Cocoa editors: the strict
  create → set_scale → get_size → set_parent → show sequence, with no editor
  left behind on any refusal; hide → destroy on close; the destructor closes
  an open editor first. The parent view crosses the layer as void* —
  plugins/ never includes Cocoa. Half an extension is treated as none, like
  params and state before it.
- Open Editor in the mixer's insert submenu; the shell owns one NSWindow per
  slot. Because instances live for their slot's lifetime (D-031), an editor
  window survives graph rebuilds; when a slot leaves the project the shell
  closes its window BEFORE the retain pass disposes the instance, so the
  plugin is told while it is still alive.
- The test gain plugin grew a recording gui extension — it draws nothing and
  records the host's calls, which is what a headless bridge test can
  honestly verify.

**Tests**

- 2 new cases (411 total, 169,335 assertions, green in Debug and Release):
  the full open/refuse-double-open/close/reopen/destruct-while-open cycle
  with the reported size; nullptr parents and editor-less plugins refused
  politely.

### Phase 13 (part 9) — Instances outlive graphs — 2026-08-15

**Changed**

- `PluginInstanceManager` now owns live instances keyed by SLOT id;
  `PluginNode` borrows (docs/DECISIONS.md D-031). Until now every rebuild —
  every added note — created a fresh instance and silently reset the
  plugin's live state. Now the same slot gets the same instance across
  rebuilds; a changed sample rate or block size recreates it but carries
  the state blob across; a changed plugin uid starts fresh.
- Instances are disposed only by the retain pass the shell runs AFTER the
  engine swaps to the rebuilt graph, with the slots the project still
  contains — so disposal cannot race the audio thread, and a bypassed slot
  keeps its instance and state.
- `createInsert` takes the slot key; `instanceFor`/`liveInstanceCount`
  expose the held instances (the editor bridge will need the former).

**Tests**

- 4 new cases (409 total, 169,316 assertions, green in Debug and Release):
  live state survives a rebuild; disposal only when the slot leaves the
  project; a device change carries state across re-activation; a replaced
  plugin starts fresh, proven by behaviour rather than pointer identity.

### Phase 13 (part 8) — Plugins reach the user: scan, add, bypass, remove — 2026-08-15

**Added**

- `app/commands/PluginCommands` — AddInsertCommand (slot id minted once, so
  redo keeps it and automation lanes stay valid), RemoveInsertCommand (the
  whole slot comes back in PLACE with its stateFile — chain position is
  audible order), SetInsertBypassedCommand (a no-op refuses to occupy an
  undo step). Every insert edit is a command (CLAUDE.md §26).
- The mixer strip's context menu grew the insert chain: Add Insert (from the
  scanned catalogue, by display name), and per slot Bypass (checkmarked) and
  Remove. The view knows menus, not catalogues — the shell hands it the list.
- File > Scan Plugins…: scans a chosen directory (defaulting to the user's
  CLAP folder) through the out-of-process scanner, persists plugins.tsv, and
  refreshes the menu. The scanner binary now rides inside INCDAW.app, so
  scanning works wherever the app is installed — a crashing plugin still
  costs the child process, never the DAW.

**Tests**

- 4 new cases (405 total, 169,280 assertions, green in Debug and Release):
  add/undo/redo with a stable slot id; refusal without occupying history;
  remove restoring position and stateFile; bypass round trip and the no-op
  refusal.

### The project as a document — Open, Save, Save As in the application — 2026-08-15

**Added**

- A File menu: Open… (Cmd-O), Save (Cmd-S), Save As… (Shift-Cmd-S). This
  closes the long-standing largest gap: ProjectFile worked and was tested
  since Phase 4, but no menu action ever called it — everything a session
  produced was lost on quit.
- Save captures live plugin state FIRST (docs/PLUGIN_HOST.md §6), so the
  recorded stateFile paths land in the project.json the same save writes.
- Open loads IN PLACE into the project object every view holds a pointer to,
  clears undo history (it no longer applies), adopts the loaded tempo into
  the transport at the device's rate, repoints every view at the loaded
  content, rebuilds the graph, and hands hosted plugins their state blobs
  back once the graph that owns them exists. A migrated older format is
  reported; a directory that is not a package is refused by name.

**Known limits (recorded, not hidden)**

- No dirty-state prompt on quit yet, no autosave, no recent-projects menu.
  Closing without Cmd-S still loses unsaved work — the difference is that
  saving is now possible.

### Phase 13 (part 7) — Hosted plugin latency joins delay compensation — 2026-08-15

**Added**

- `ClapInstance` queries CLAP_EXT_LATENCY once, while activated, at creation
  (capped at ten seconds against hostile reports); `PluginNode::latencyFrames`
  surfaces it, and the graph's existing PDC (Phase 10) does the rest — no
  plugin-specific code in the engine.
- `tests/plugins/TestLatencyPlugin.cpp` — the suite's third CLAP plugin: a
  TRUE 64-frame delay that also reports 64, so compensation is tested against
  a plugin that behaves like a real look-ahead processor.

**Tests**

- 3 new cases (401 total, 169,249 assertions, green in Debug and Release):
  the instance reports its latency (and none means zero); the exit criterion —
  an impulse split between a hosted latent path and a direct path arrives
  once, aligned, at the plugin's latency, with the un-compensated comb shown
  first; a project whose master insert is latent reports the graph's total
  latency.

### Phase 13 (part 6) — Plugin state: capture, the package, and restore — 2026-08-15

**Added**

- `engine::StateIO` — a pure interface for nodes whose state is an opaque
  blob, with `Node::stateIO()` following the `parameterSink()` capability
  pattern (docs/DECISIONS.md D-030).
- `ClapInstance` implements it over CLAP_EXT_STATE with stack-local stream
  adapters; a hostile plugin is held to a 64 MB save cap, and half an
  extension (save without load) is treated as none.
- `CompiledProjectGraph::insertSlots/insertStates` + `insertStateFor` — how
  project save reaches a live insert's blob. Filled only on a successful
  compile, so a failed build can never hand out dangling carriers.
- `project/PluginStateFiles` — `capturePluginState` writes each live
  insert's blob to `plugins/insert-<slot-id>.state` in the package
  (stage-and-rename, like every package write) and records the relative path
  in `PluginSlot::stateFile`, which already travels in project.json — no
  format change. `restorePluginState` hands blobs back after a compile.
- The test gain plugin gained CLAP_EXT_STATE: its gain as 8 raw bytes,
  strict about shape and range so the host's rejection path is honest.

**Behaviour**

- Capture before `ProjectFile::save`; restore after compiling a loaded
  project. Failures are warnings, never a failed save: a plugin that will
  not save keeps its previous blob; a plugin that rejects its blob plays its
  defaults, named to the UI; a missing plugin's slot (and its blob file) is
  simply not touched, so installing the plugin later restores the session.
- The shell does not call capture/restore yet — the application still has no
  save/open action, which remains the standing gap.

**Tests**

- 5 new cases (398 total, 168,719 assertions, green in Debug and Release):
  instance-level state round trip incl. garbage rejection; the exit
  criterion — state survives save, load and recompile bit-exactly; the
  missing-plugin placeholder rule; a rejected blob as a named warning with
  the plugin at defaults; an unreadable state file as a warning, not a
  failed load.

### Phase 13 (part 5) — Plugin parameters: discovery and the event queue — 2026-08-15

**Added**

- `engine::ParameterSink` — a pure interface for parameter targets that are
  not mixer strips, with an optional `Node::parameterSink()` accessor so the
  graph compiler can bind automation onto a node it only knows as
  `engine::Node` (docs/DECISIONS.md D-029).
- `ParameterRegistry::Entry::apply` is now a variant: `StripApplier`
  (unchanged) or `SinkApplier`. `registerPluginParameters` turns a discovered
  parameter list into generic normalised-to-plain appliers, keyed
  `plugin:<uid>:<param-id>`; the lane's `targetEntity` (the insert slot id)
  picks the instance, so two instances of one plugin share entries.
- `ClapInstance` discovers `CLAP_EXT_PARAMS` at creation into format-agnostic
  `plugins::PluginParameterInfo` (skipping non-automatable and malformed
  parameters as hostile input), and implements `ParameterSink` with a
  preallocated lock-free queue drained into the block's
  `clap_event_param_value` input list — a value never reaches the plugin
  through a call, which is what the CLAP concurrency contract demands.
- `PluginInstanceManager::parametersFor` — discovery cached per plugin type.
- The shell owns a `ParameterRegistry` and registers discovered parameters in
  the insert factory, between instance creation and lane binding in the same
  compile.
- The test gain plugin's gain is now a real CLAP parameter (plain 0..2,
  default 0.5 keeps the -6 dB the earlier tests assert).

**Behaviour**

- Delivery is per-block (`time = 0`), matching the AutomationNode's grain.
  The host does not smooth plugin parameters: that is the plugin's job, and
  host-side smoothing would break stepped parameters.
- A full event queue drops values; automation writes every block, so the
  loss heals one block later.
- `params->flush()` is not yet called while the engine is idle (lands with
  the editor bridge, PLUGIN_HOST §7).

**Tests**

- 7 new cases (393 total, 162,525 assertions, green in Debug and Release):
  registry generalisation and stepped mapping against a recording sink;
  discovery in plain terms; per-type discovery caching; event delivery and
  last-value-wins ordering through a real process call; the exit criterion —
  an automation lane drives a hosted plugin's parameter to unity gain through
  the generic subsystem, rendered under the realtime guard with zero
  allocation violations; mismatched key/target pairs skipped as data.

### Phase 13 (part 4) — Inserts in the compiled graph — 2026-08-15

**Added**

- `plugins/PluginInstanceManager` — owns loaded plugin libraries for the
  application's lifetime, keyed by path. Two instances of a plugin share one
  opened binary, and no binary is closed while the app runs: a PluginNode's
  instance calls back into its library when destroyed, and graphs are retired
  asynchronously.
- `GraphCompileOptions::insertFactory` — a mixer node's `inserts` now compile
  into a chain in front of its strip. The factory returns an `engine::Node`,
  so `project/` places hosted plugins in the graph without including a plugin
  header (docs/DECISIONS.md D-028).
- The compiler keeps separate input and output indices per mixer node: signal
  enters at the head of the insert chain and leaves at the strip. Channels,
  audio tracks, sends and the input monitor all now arrive ahead of the
  destination's plugins rather than behind them.
- The shell loads the plugin catalogue from Application Support at launch
  (touching no plugin binary) and injects the factory into every rebuild.

**Behaviour**

- Inserts run pre-fader: a fader move does not change what a compressor hears.
- A bypassed slot is not instantiated at all. A slot that cannot be built is a
  pass-through plus a named compile warning — a missing plugin costs its own
  slot, never the rest of the mix.

**Tests**

- 9 new cases (386 total, 160,673 assertions, green in Debug and Release):
  chain order, pre-fader placement proven with a non-linear insert (and shown
  to fail when the chain is wired after the strip), bypass, one node per slot
  regardless of source count, unbuildable slot, host-less build, and the
  end-to-end case — a real hosted CLAP as a mixer insert, allocation-free
  under the realtime guard.

### Phase 13 (part 3) — A plugin in the graph — 2026-08-15

**Added**

- `plugins/PluginNode` — a hosted plugin as a render-graph node: sums its
  inputs like a strip (an insert processes the mixed signal), then the
  plugin runs in place on the node's output. Mono graphs pass through
  rather than handing the plugin two aliases of one buffer. plugins now
  links engine (same layer rank; the checker allows it) so the node can
  derive `engine::Node`.
- `platform/ChildProcess` — fork/exec/capture moved behind the platform
  boundary where the layering checker rightly demanded it; `PluginScan`
  now contains no OS API.
- Test: sine -> plugin insert -> master renders bit-identically to the
  same chain with the gain baked in, under the realtime guard with zero
  allocations.

### Phase 13 (part 2) — The registry and its blacklist — 2026-08-15

**Added**

- `plugins/PluginRegistry` — the persisted catalogue: startup loads it and
  touches no plugin binary at all. Scanning goes through the out-of-process
  scanner; the cache key is (size, mtime), so an unchanged library —
  including a blacklisted one — never spawns another child. A crashed or
  failed library is blacklisted WITH its reason and skipped until the user
  clears it; clearing erases the entry so the next scan genuinely retries.
  Versioned TSV persistence; an unknown version is refused, not guessed at.
- `tests/unit/PluginRegistryTests.cpp` — cataloguing + blacklisting, the
  zero-rescan cache guarantee, mtime-triggered rescan, blacklist retry,
  file round trip, version refusal.

### Phase 13 (part 1) — CLAP hosting foundation — 2026-08-15

**Added**

- `third_party/clap` — the CLAP C ABI, pinned at 1.2.6, MIT (D-027);
  gitignored with refetch instructions, like doctest.
- `platform/SharedLibrary` — dlopen behind the platform boundary
  (RTLD_LOCAL: one plugin's symbols must not collide with another's).
- `plugins/clap/ClapLibrary` — entry resolution (bundle or flat dylib),
  version check, factory enumeration with hostile-input scepticism, and
  `ClapInstance`: create/init/activate/start, stereo processing with valid
  empty event queues, ordered teardown.
- `incdaw-pluginscan` + `plugins::scanOutOfProcess` — the disposable
  scanner: fork/exec (an argv never meets a shell), descriptors parsed from
  its report, and a died-of-signal child classified as `crashed` — the
  outcome the mechanism exists for.
- The test suite's own plugins: `incdaw-testgain.clap` (a real, minimal,
  well-behaved CLAP gain) and `incdaw-testcrash.clap` (segfaults in entry
  init). The host cannot be tested against third-party binaries and must
  not be tested against nothing.
- `tests/unit/PluginHostTests.cpp` — scan/load/process round trip through
  the gain plugin, clean failure on unknown ids, out-of-process scanning,
  and the exit-criterion seed: **a plugin that crashes on load kills the
  scanner and the host finishes the test.**

Next in Phase 13: the registry + blacklist persistence, the PluginInstance
graph node (audio through the mixer), parameter discovery bridged to the
generic automation subsystem, state save/load, and editor hosting.

### Phase 12 (part 8) — The Audio Logger — 2026-08-15

**Added**

- `engine/audio/AudioLogger` — the master's last 60 seconds, continuously:
  a keep-newest circle (the recorder's ring is keep-oldest) fed at the end
  of every rendered block, so a grab retrieves exactly what was heard.
  `grab` snapshots the monotonic write count, copies, and trims what the
  writer overran mid-copy — at worst milliseconds short at the oldest end,
  never torn, never a lock near the callback. Logging is wait-free and
  allocation-free under the guard.
- Audio menu: an Audio Logger toggle (off by default — 23 MB and a "was
  that being kept?" question the user should answer, not inherit) and
  "Grab Last 60 Seconds", which writes the window to the recordings folder
  and lands it as a clip whose last frame sits at the playhead — one undo,
  like every other landing.
- `tests/unit/AudioLoggerTests.cpp` — ordered wrap-around, partial fills,
  disabled-keeps-nothing, allocation-free logging, and an engine-level
  check that the grab equals the rendered output bit for bit.

An input-side pre-record buffer is deliberately separate, deferred work:
conflating it with the master logger would keep microphone audio while the
user believed only playback was kept.

### Phase 12 (part 7) — Loop and punch recording — 2026-08-15

**Added**

- `RecordingSession::computeSlices` — the capture stays one continuous
  file; placement cuts it against the loop it was recorded under. Each pass
  over the loop becomes its own slice (same asset, advancing source
  offset), stacked on the track with every pass but the newest muted —
  takes ready for comping rather than playing all at once. A take armed
  before the loop runs linearly into it, then wraps.
- The linear anchor map folds a wrapped take; `finish` unfolds it by
  choosing the loop pass that puts the take's start where the playhead
  stood at arm time. Engaged only when the contract sampled at arm held:
  same loop range, loop still on, and no explicit seek — `Transport` now
  counts seeks so a user jump (which arithmetic cannot reconstruct) is
  distinguishable from a wrap (which it can). Any interference falls back
  to the straight single-clip placement.
- Punch: a placement decision over a continuous capture, not a capture
  gate. "Punch to Loop Range" in the Audio menu trims every slice to the
  loop window with honest file offsets; a take that never enters the window
  lands nothing (the file still exists on disk).
- `InsertRecordedTakeCommand` lands one clip per slice — one asset, N
  views of it — as a single undo; redo restores identical ids.
- `tests/unit/LoopRecordingTests.cpp` — straight, wrapped, armed-pre-loop,
  punch trims, punch-excludes-everything, multi-clip landing/undo/redo,
  and the every-file-frame-lands-exactly-once invariant.

### Phase 12 (part 6) — Input monitoring — 2026-08-15

**Added**

- `engine/audio/InputMonitorNode` — plays the live input through the graph:
  drains the engine's monitor ring into the master strip. The ring bridges
  the input and output clock domains; the node caps the drift (a backlog
  past four blocks is skipped so monitoring latency cannot grow without
  bound) and plays silence on underrun rather than waiting.
- `AudioEngine` — a monitor ring allocated once for the engine's lifetime
  (graph nodes keep the pointer across device restarts); the capture
  callback interleaves into it while monitoring is enabled, wait-free.
- Audio menu: Monitor Input toggle — opens the input on demand (same flow
  as record arming) and rebuilds the graph, since the monitor node exists
  exactly when monitoring is on. Graph monitoring, stated honestly: input
  latency + one ring hop + output latency; direct hardware monitoring would
  be a device-layer feature.
- `tests/unit/InputMonitorTests.cpp` — pass-through fidelity, mono fan-out,
  underrun silence, the drift cap, allocation-free path under the guard,
  compiler wiring through the master.

### Phase 11b — Automation placement and recording — 2026-08-15

**Added**

- `AutomationNode::Binding` — a half-open tick window; the binding simply
  does not evaluate outside it (D-026). Clip semantics fall out: nothing
  before the clip starts, the last written value holds after it ends.
- `ProjectGraphCompiler` — every placement becomes a windowed binding with
  the lane's points shifted into position: automation clips, and pattern
  clips whose pattern lists lanes (`Pattern::automationLanes` is finally
  evaluated). A placed lane plays only through its placements — a muted
  placement silences it rather than promoting it to global; only an
  unplaced lane plays everywhere (11a behaviour).
- `app/AutomationWriteSession` + `WriteAutomationCommand` — write-mode
  recording: armed fader/pan moves are captured with their transport ticks,
  thinned to where the envelope actually bends (a straight ramp keeps two
  points), and landed undoably. Writing over an existing lane replaces only
  the written range; a new lane arrives with an automation clip and a track,
  as one undo — the recorded-take landing pattern.
- Mixer: `onParameterEdited` reports moves in the registry's normalised
  terms (both sides share the cubic fader law, so a pass replays
  identically). Audio menu: Write Automation toggle.
- Playlist: automation clips are visible, movable, resizable, with their
  envelope drawn in the clip body.
- `tests/unit/AutomationClipTests.cpp` — window semantics, muted placement,
  global fallback, pattern-carried lanes, recorded-pass landing/undo/redo,
  range-preserving overwrite, loop-wrap restart, thinning.

**Phase 11 is COMPLETE.** Deferred: touch/latch modes, loop-aware overdub,
a dedicated point-editing surface, copy/paste and scaling UI (ROADMAP).

### Phase 9b — Audio clips are first-class playlist citizens — 2026-08-15

**Added / changed**

- `MoveClipsCommand` / `ResizeClipsCommand` — audio clips move and resize on
  the tick grid with the delta converted through the tempo map at the clip's
  own position (resize converts at the clip's END, where the handle is);
  undo restores frame snapshots, because tick->frame does not invert exactly
  across tempo changes. A pure track move never round-trips the position.
- Fixed a latent merged-drag bug: redo of a merged move replayed only the
  gesture's FIRST step. `mergeWith` now accumulates the requested deltas
  too, so redo replays the whole gesture (pinned by a test).
- Playlist: audio clips draw their waveform in the clip body (mono-folded,
  from `WaveformOverview`), cached per asset, invalidated by the host after
  any edit/undo that may have rewritten a file. Resize handle re-enabled.
- `ProjectGraphCompiler` — `clip.normalize` is honoured: the clip content's
  peak (its source range) folds into the placement gain at compile time, so
  the node stays one multiply per sample. Streamed clips defer with a
  warning (exact peak would cost a file pass per rebuild).
- `tests/unit/AudioClipEditingTests.cpp` — move/resize/undo exactness,
  mixed-selection clamping, merged-drag redo, minimum-length clamp; plus a
  pre-mixer normalize equivalence test in RecordingPlacementTests.

**Phase 9 is COMPLETE**: a full arrangement (pattern + audio clips) plays
back sample-accurately, and clip gain and normalize are applied pre-mixer
and recalled from the project file. Deferred items are listed in ROADMAP.

### Phase 12 (part 5) — The audio editor — 2026-08-15

**Added**

- `engine/audio/AudioEdits` — the editor's verbs as pure region operations:
  gain, peak, normalize (refuses silence), reverse, silence, linear fades,
  trim. Half-open regions, clamped, no I/O anywhere near the DSP.
- `engine/audio/WaveformOverview` — min/max buckets built through the
  streaming reader in chunks (or from memory after an edit): an hour-long
  file yields its waveform without being resident.
- `app/commands/AudioEditCommands` — destructive edits, undoably:
  `EditAssetRegionCommand` snapshots the region before rewriting the file
  (undo restores bit-exactly; redo writes the recorded result rather than
  re-applying — gain would compound); `TrimAssetCommand` keeps the cut head
  and tail and reassembles the original file, length and asset metadata
  included. Edited audio renders as float32.
- `ui/macos/AudioEditorView` — the editor pane: waveform per channel,
  drag-select (double-click selects all), scroll pans, Cmd+scroll zooms
  around the cursor. Fourth editor segment; ⌘6 in the View menu.
- Playlist: double-clicking an audio clip opens its asset in the editor.
- Audio menu: Trim to Selection, Normalize, Reverse, Silence, Fade In/Out,
  Gain ±3 dB — each runs on the selection (or the whole file, the Edison
  convention; trim requires a selection), then the waveform reloads and the
  playback graph rebuilds so the edit is immediately audible. Undo/redo of
  an audio edit is caught from housekeeping and refreshes both.
- `tests/unit/AudioEditsTests.cpp` — region exactness, normalize/reverse
  semantics, clamping, overview file-vs-memory equality, and bit-exact undo
  of both command classes including trim's file-length round trip.

Still to come in Phase 12: input monitoring, loop/punch recording, the
pre-record buffer / Audio Logger, and editor polish (markers, regions,
cut/copy/paste between files, spectral view).

### Phase 12 (part 4) — The disk streamer — 2026-08-15

**Added**

- `engine/audio/WavStreamReader` — random-access WAV decode: a streaming
  chunk walk parses the header without loading the body, and `readAt`
  decodes any frame range through the same `WavBytes.h` decoder `WavFile`
  uses (`decodeSample` extracted so the two readers cannot disagree).
- `engine/audio/AudioStream` — one streamed clip's window: two segments
  leapfrogging ahead of the play position, each under its own seqlock. The
  realtime read is wait-free; what the window cannot serve is zero-filled
  and counted, never waited for. Seeks are just a requested position the
  next service pass moves the window to (D-025).
- `engine/audio/DiskStreamer` — one background thread services every live
  stream; streams are held weakly and die with their graphs. `serviceOnce`
  is public so tests drive servicing deterministically instead of sleeping.
- `AudioClipNode` — clips now play from either a preloaded buffer or a
  stream through one gain-and-fade path; `prepare` sizes the fetch scratch.
- `ProjectGraphCompiler` — assets longer than
  `GraphCompileOptions::streamingThresholdFrames` (default 30 s) stream, one
  stream per clip, windows prefilled at compile time so a rebuilt graph
  starts warm. The app owns the `DiskStreamer`.
- `tests/unit/DiskStreamingTests.cpp` — reader-vs-WavFile slice equality in
  all formats, bit-exact streamed playback across forced refills, seek
  refill, starvation honesty (counted silence, no blocking), allocation-free
  read path under the realtime guard, and streamed == preloaded through the
  compiled graph.

Still to come in Phase 12: input monitoring, loop/punch recording
(per-segment anchoring), the pre-record buffer / Audio Logger, and the
audio editor.

### Phase 12 (part 3) — Recording lands in the timeline — 2026-08-15

**Added**

- `engine::TimelineAnchor` — every rendered block publishes its (host time,
  timeline frame) correlation through a seqlock (D-024); the audio thread
  never blocks, readers retry the rare torn read.
- `engine/audio/AudioClipNode` — audio clips are audible: one node per audio
  track plays preloaded planar clips at their exact timeline frames with gain,
  mute and linear fades. Pan/normalize/reverse/pitch/stretch deliberately not
  yet (9b polish).
- `project/RecordingSession` — arm-to-placement coordinator: starts the
  recorder from the engine's own figures, and on finish maps the take's
  latency-compensated start through the anchor onto the timeline (or the
  stopped playhead). Returns a Placement; it never mutates the Project.
- `project::clipStartTicks` / `clipLengthTicks` — the D-013 accessor: clip
  placement resolved by type, so the playlist can lay frame-anchored audio
  clips on its musical grid without mixing time bases.
- `app/commands/RecordingCommands` — `InsertRecordedTakeCommand`: take file →
  AudioAsset → audio clip on the first audio track (created if none), one
  undo for the whole landing; redo restores identical ids. The file on disk
  is deliberately not deleted by undo.
- `ProjectGraphCompiler` — audio tracks compile: assets decoded once and
  shared, wrong-rate or unreadable assets stay silent with a warning
  (`CompiledProjectGraph::warnings`), clips feed the track's mixer node.
- INCDAW app: plain `R` toggles recording; the input device opens on first
  arm (the microphone is never opened unasked), a Bluetooth HFP mic failure
  falls back to output-only with the reason in the status line; `● REC` with
  elapsed time while recording; the finished take appears in the playlist.
- Playlist: audio clips are visible, selectable and deletable. Moving and
  resizing them is deferred to 9b (frame-anchored moves need per-clip tempo
  math) and the commands skip them explicitly rather than corrupting them.
- `incdaw-audiocheck --record` now runs the whole chain (RecordingSession +
  anchor) and reports where the take was placed.
- `tests/unit/RecordingPlacementTests.cpp` — anchor arithmetic, sample-exact
  clip playback, fades/gain/offset/mute, compiler placement through the
  master, wrong-rate refusal, command undo/redo id stability, serialization
  round trip of a recorded take.

Verified on hardware: recording from the MacBook microphone while the
transport rolled the arpeggio — 3 s take, 0 drops, 0 overruns, 0 realtime
allocations, placement computed against the rolling transport.

Still to come in Phase 12: the disk-streaming reader, input monitoring,
loop/punch recording (per-segment anchoring), the pre-record buffer / Audio
Logger, and the audio editor.

### Phase 12 (part 2) — Input capture and recording — 2026-08-15

**Added**

- `platform/AudioDevice` — input capture. `AudioIOCallback::captureAudioBlock`
  (default no-op), separate input device selection (`"default"` sentinel — an
  empty identifier still means "never open the microphone unasked"), input
  latency/safety-offset/channel reporting, `totalInputLatencyFrames`.
- `platform/macos/CoreAudioDevice` — a second IOProc on the input device (on
  Macs the microphone is a separate HAL device); duplex devices use the main
  proc's input arguments. Input scratch sized from the input device's own
  maximum; record-and-restore of the input device's buffer size; a nominal
  rate the input cannot match is a hard, explained failure (D-023).
- `engine/audio/WavStreamWriter` — incremental WAV writing for takes: streams
  blocks as they arrive, patches the RIFF sizes on finalize, byte-identical to
  `WavFile::write` (both share `WavBytes.h`). An unfinalized file probes as an
  empty take, not garbage.
- `engine/core/SampleRingBuffer` — SPSC bulk sample ring, runtime capacity,
  two memcpys and one release store per call.
- `engine/audio/AudioRecorder` — realtime-safe capture to WAV: wait-free
  interleave into the ring on the capture thread, polling writer thread
  draining to disk, whole-frame drops counted and reported, take start
  reported with the device's input latency subtracted.
- `engine/AudioEngine` — `setCaptureSink` (atomic, same pattern as the graph
  swap), input passthroughs, capture forwarding under the realtime guard.
- `incdaw-audiocheck --record [--input UID] [--take PATH]` — hardware
  verification of the capture path.
- `tests/unit/WavStreamWriterTests.cpp`, `tests/unit/AudioRecorderTests.cpp` —
  including the Phase 12 exit criterion: a simulated loopback whose recorded
  audio lands sample-accurately with compensation applied, and exactly
  `latency` frames late with it removed.

Verified on hardware (AirPods output + MacBook microphone — two devices, two
clocks): 2 s take, 0 dropped frames, 0 overruns, 0 realtime allocations. The
AirPods HFP microphone (24 kHz) is correctly refused with both rates named.

Still to come in Phase 12: the disk-streaming reader, recording into the
timeline (a take becoming an audio clip), monitoring, and the audio editor.

### Phase 12 (part 1) — WAV codec — 2026-08-15

**Added**

- `engine/audio/WavFile` — RIFF/WAVE read, probe and write. PCM 16/24/32 and
  IEEE float 32; WAVE_FORMAT_EXTENSIBLE unwrapped; chunk walking that survives
  LIST/bext/junk chunks and honours the pad byte on odd sizes; sign-correct
  24-bit decode. `probe` fills metadata without decoding — what the browser and
  the relinker need.
- `tests/unit/WavFileTests.cpp` — bit-exact float round trip, PCM round trips
  within one quantisation step, probe, spliced-chunk survival, garbage refusal.

The gate for Phase 9b (audio clips), 11b (automation recording) and the audio
editor. Still to come in Phase 12: input capture, the disk streamer, recording
into the timeline, the measured loopback exit criterion, and the editor.

### Phase 11a — Automation: the generic subsystem — 2026-08-15

**Added**

- `engine/automation/AutomationSequence` — sorted points, binary-search
  evaluation: linear, hold, smoothstep, exponential, tension.
- `engine/automation/AutomationNode` — evaluates every lane per block inside
  the graph; dies with it, so appliers cannot dangle.
- `project/ParameterRegistry` — key → normalised-value-to-strip mapping;
  "volume" (through the fader's cubic law) and "pan" built in.
- `app/commands/AutomationCommands` — lane add/remove, wholesale point edits
  (points stay sorted), merged drags.
- `tests/unit/AutomationTests.cpp` — 8 cases, including the exit criterion via
  an unknown key, graph-driven volume/pan, realtime safety, command round trips.

**Fixed**

- System-wide crackle: CoreAudioDevice forced the shared device's buffer to 256
  and never restored it (now recorded and restored in close; default request is
  512), and the render graph was compiled for the current buffer size while a
  shared device can deliver its maximum — AirPods report 15..960 — so oversized
  blocks were truncated into a duty-cycled buzz (graphs now compile for
  `maxServiceableBlockSize`).

**Known gaps**

- No automation UI, no automation clips, no recording modes (11b). Pattern
  automation lanes still serialize without being evaluated. **Phase 11 is not
  complete.**

### Phase 10 — Mixer, routing and delay compensation — 2026-08-14

**Added**

- `engine::GraphBuilder` delay compensation: delay lines inserted on short paths
  into any summing node (D-019). `setDelayCompensationEnabled` exists so the
  compensation can be tested against its own absence.
- `engine/dsp/DelayLineNode` — fixed whole-sample delay, allocation-free while
  rendering.
- `engine/dsp/MixerStripNode` — summing, polarity, constant-power pan, fader,
  mute and metering in one pass (D-020, D-021).
- `engine/core/Smoother` — the click-free ramp, lifted out of `GainNode`.
- `engine/core/LevelMeter` — peak and RMS over a 300 ms window, published with a
  relaxed atomic store.
- `app/commands/MixerCommands` — add/remove/rename mixer nodes, volume, pan,
  mute, solo, polarity, channel routing, connect/disconnect, send level.
- `ui/macos/MixerView` — strips, cubic fader law, pan, M/S/Ø, live meters.
- A Mixer pane (⌘3) and a Play/Stop menu item.
- `tests/unit/MixerTests.cpp` — 19 cases: the PDC exit criterion, non-aligned
  latencies, compensation through a chain, the delay line, the pan law, strip
  behaviour, metering, realtime safety, a 64-strip performance measurement, plus
  routing, sends, solo/mute and command round trips.
- `Project::insertMixerNode` / `insertRouting` / `removeMixerNode` /
  `removeRouting` / `indexOf*` / `findRouting`.

**Changed**

- The signal path is now instrument → channel strip → mixer track → sends/buses
  → master. Channel pan is applied for the first time.
- A routing cycle is reported in the status line and leaves the previous graph
  playing, rather than failing silently.

**Known gaps**

- Insert chains are empty (Phase 13, Phase 15), sidechain edges compile to
  nothing, pre-fader sends behave as post-fader.
- LUFS is architecturally ready but not implemented.

### Phase 9a — Playlist: the pattern arrangement — 2026-08-14

**Added**

- `app/commands/TrackCommands` — add, remove (with the track's clips), rename,
  mute, solo, height.
- `app/commands/ClipCommands` — add, remove, move (mergeable), resize
  (mergeable), duplicate, mute. Clips are addressed by id, not by index.
- `app/PlaylistModel` — culling, hit testing, resize handles, box selection and
  snap, headless.
- `ui/macos/PlaylistView` — ruler, track headers, clips, drag/resize/paint,
  ruler seeking, playhead.
- A Pattern/Song transport mode, and a View menu binding the editors to ⌘1/⌘2
  and the modes to ⌘3/⌘4.
- `Project::insertTrack` / `insertClip` / `removeTrack` / `removeClip` /
  `indexOfTrack` / `indexOfClip` / `findClip`.
- `tests/unit/PlaylistTests.cpp` — 13 cases, including the roadmap exit
  criterion through the compiled graph, and a recompile measurement.

**Changed**

- `project::compileArrangement` honours track mute and solo (D-018).
- A new project opens with one track and the pattern placed on it, so song mode
  plays something.
- Editor panes now resize with the window; they previously kept their initial
  size and left a dead margin down the right edge.

**Known gaps**

- No audio clips, and therefore no clip gain or normalize: Phase 9b, which needs
  a file reader and a streamer. **Phase 9 is not complete.**
- No automation clips (Phase 11), no fades, crossfades, stretch or reverse.
- Live mouse interaction was not verified this session; the headless tests cover
  the edits.

### Phase 8b — Channel Rack, pattern list, step sequencer — 2026-08-14

**Added**

- `app/commands/ChannelCommands` — add, remove, rename, mute, solo, volume
  (mergeable), step key. Removing a channel takes its content in every pattern
  with it and gives all of it back.
- `app/commands/PatternCommands` — add, duplicate, remove, rename, length,
  swing.
- `app/commands/StepCommands` — `ToggleStepCommand` and `noteAtStep`. A step is
  an ordinary note (D-016).
- `app/ChannelRackModel` — rack geometry and hit testing, drawn by nobody and
  therefore tested headlessly.
- `ui/macos/ChannelRackView` — channels, mute/solo, volume, and the step grid,
  with drag-painting and a playhead column (D-015).
- `ui/macos/PatternListView` — select, add, duplicate, rename, remove.
- `Channel::stepKey` — the pitch a channel's steps are written at.
- `Project::insertChannel` / `insertPattern` / `removeChannel` / `removePattern`
  / `indexOfChannel` / `indexOfPattern` — what undo needs to put an entity back
  where it was, with the identity it had.
- `tests/unit/ChannelRackTests.cpp` — 13 cases: command round trips, mute and
  solo reaching the compiled graph, steps visible to the Piano Roll, rack hit
  testing.
- `tests/fixtures/v1.1/` — the 1.1 fixture, now that 1.1 is no longer current.

**Changed**

- Project format 1.1 → 1.2, additive: a 1.1 file has no `stepKey` and reads back
  as middle C. Both fixtures load.
- The transport loops the selected pattern rather than `patterns()[0]`.
- Retargeting the Piano Roll clears its selection, since note indices belong to
  one channel's list in one pattern.
- Space and ⌘Z work from any pane.

**Known gaps**

- Drag-painting steps leaves one undo entry per cell rather than one per stroke.
- The project is still never saved from the UI.
- Channel colour and step key have commands but no UI to reach them.

### Phase 8a — Pattern system: model and compilation — 2026-08-14

**Added**

- `project::PatternChannelContent` — a pattern now holds its notes per channel,
  with an optional per-channel loop length for polymetric patterns (D-012).
- `Pattern::swing` / `swingGrid` — shuffle resolved at compile time, applied to
  notes exactly on an odd grid line (D-014).
- `Clip::startTick` / `lengthTicks` / `sourceOffsetTicks` — musical placement,
  authoritative for pattern and automation clips (D-013).
- `project::compileArrangement` — every note a channel plays across the
  arrangement's pattern clips. A clip shorter than its pattern trims it, a
  longer one repeats it, and notes are cut at the clip boundary.
- `project::compileProjectGraph` — the Project → render graph seam, with an
  injectable `InstrumentFactory`, per-channel gain, and project-wide solo.
- `tests/unit/PatternTests.cpp` — 15 cases including the Phase 8 exit criterion
  and a recompile-cost measurement.

**Changed**

- Project format 1.0 → 1.1. Pattern files store `channels[]`; clips store tick
  placement. 1.0 files are migrated on load and the 1.0 fixture still passes.
- Note commands address a (pattern, channel) pair; `PianoRollModel` and
  `MidiCapture` take an event list rather than a `Pattern`.
- `ui/macos/main.mm` no longer assembles the render graph by hand.

**Known gaps**

- No Channel Rack, pattern list, or step sequencer UI yet (Phase 8b): the app
  still opens one pattern on one channel. *(Closed by Phase 8b.)*
- No `tests/fixtures/v1.1/` fixture yet — required before 1.1 ships.
  *(Closed by Phase 8b.)*

### Phase 0 — Research and architecture — 2026-08-14

**Added**

- Git repository initialised on branch `main`, with `.gitignore`.
- `docs/ARCHITECTURE.md` — layer model, threading model, data model, command
  architecture, engine boundary.
- `docs/DECISIONS.md` — decision log D-001…D-010.
- `docs/ROADMAP.md` — phases 0–20 with testable exit criteria.
- `docs/REQUIREMENTS.md` — functional scope and FL Studio 2026 reference notes.
- `docs/AUDIO_ENGINE.md` — device layer, realtime scheduling, PDC, offline
  render, correctness requirements.
- `docs/PLUGIN_HOST.md` — format support, isolation strategy, parameter and
  state systems.
- `docs/PROJECT_FORMAT.md` — `.incdaw` v1.0 package format, versioning,
  migration, media handling.
- `docs/TESTING.md` — test levels, specialised tests, stated coverage gaps.
- `docs/PERFORMANCE.md` — targets, instrumentation, method.

**Decided** (see docs/DECISIONS.md)

- C++20 core, CMake + Ninja build, direct CoreAudio/CoreMIDI, `os_workgroup`
  realtime scheduling, AppKit + Metal UI.
- Plugin formats: CLAP → AU → VST3. VST2 excluded (SDK not licensable).
- Closed-source; permissive dependencies only. JUCE rejected on licensing.
- macOS first; Windows a later target behind `platform/` isolation.
- Distribution as an ad-hoc-signed, un-notarized `.dmg`.

### Phase 1 — Foundation and build system — 2026-08-14

CMake 3.28+/Ninja build, six layer libraries, warnings as errors, doctest
harness, `tools/check_layering.py` wired into ctest, and `tools/make-dmg.sh`
producing a verified, ad-hoc-signed DMG.

### Phase 2 — Audio engine foundation — 2026-08-14

CoreAudio HAL device layer, realtime-safety guard, lock-free SPSC queue,
denormal control, audio buffer pool and views, render graph with topological
sort and cycle rejection, atomic graph swap with deferred reclamation, sine
and gain nodes, callback profiler, `tools/audiocheck`.

Verified: 440 Hz through real hardware, 0 overruns, 0 realtime allocations.

### Phase 3 — Transport — 2026-08-14

Tempo map with precomputed segment frames, time-signature map, transport
state machine, block-splitting plan for loop wraps, metronome.

Verified: click events land on the tempo map's exact frame across tempo
changes, loop boundaries, and every block size from 32 to 1024.

### Phase 4 — Project model and format — 2026-08-14

Deterministic ordered JSON, the full entity model (§24-compliant, nothing
collapsed), `.incdaw` package v1.0 with staged writes, migration hook, and a
hand-written permanent v1.0 fixture.

Verified: round-trip equality, byte-identical repeat saves, corrupt-pattern
containment, newer-format refusal, missing-media survival.

**Current state**

123 test cases, 29.5k assertions passing. Phases 5-20 not started.
