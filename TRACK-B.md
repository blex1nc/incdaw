# TRACK B — "arrange" · playlist, patterns and automation

Branch: `claude/arrange` · Worktree: `.claude/worktrees/arrange`

You are one of three parallel tracks closing INCDAW's gap against FL Studio.
The full re-audited gap list is `docs/FL2026_GAP.md` §3; this brief is your
slice of it. The other two tracks are `timbre` (instruments and effects) and
`bridge` (hardware, hosting and files). You will not need their files.

## What this track is for

The playlist can place, move, split, fade, stretch and warp clips, and the
automation system records in write, touch and latch. What it cannot do is
*organise*: there are no folders, no lanes, no groups, and three clip
properties that the model stores and the compiler applies have no command able
to set them. This track turns the arrangement from a place clips go into a
place a song gets built.

## Your files

**Yours, exclusively**

- `src/project/Model.h`, `src/project/Model.cpp` — the clip, track and pattern
  side of it
- `src/project/ProjectFile.cpp` — the format, its version and its migrations
- `src/app/commands/ClipCommands.*`, `TrackCommands.*`, `PatternCommands.*`,
  `AutomationCommands.*`, `MarkerCommands.*`
- `src/app/PlaylistModel.*`, `src/app/PianoRollModel.*` where the arrangement
  needs it
- `src/ui/macos/PlaylistView.*`, `src/ui/macos/PatternListView.*`
- `tests/unit/PlaylistTests.cpp`, `AutomationTests.cpp`,
  `AutomationClipTests.cpp`, `PatternTests.cpp`, `AudioClipEditingTests.cpp`,
  `MarkerAndSplitTests.cpp`, `ProjectFormatTests.cpp`, and any new test file

**Shared — additive only:** `src/CMakeLists.txt`, `tests/CMakeLists.txt`,
`CHANGELOG.md`, `src/ui/macos/main.mm` (menu items, command registrations and
key handling for your verbs only).

**Not yours:** `src/engine/instrument/`, `src/engine/dsp/`, `src/platform/`,
`src/plugins/`, `src/project/OfflineRender.*`, `AudioEditorView.*`.

**Format warning.** You own `INCDAW_PROJECT_VERSION`. Bump it once per shipped
increment at most, ship the migration with it, and add a `ProjectFormatTests`
case proving a file written before your change still opens with the right
values. An unversioned or unmigrated change is a data-loss bug in a DAW.

## The work, in order

### B1–B4 — the verbs the model is already waiting for (do these first)

Four fields are serialized, compiled and applied today with nothing able to
set them. These are the cheapest real features in the whole gap list and they
should land before anything structural:

- **B1 `Clip::pan`** — a command, undo, and a control in the clip's context
  menu or inspector.
- **B2 `Clip::reversed`** — same. The compiler already honours it.
- **B3 `Clip::locked`** — same, plus: a locked clip refuses drag, resize,
  split and delete, and says so rather than silently ignoring the gesture.
- **B4 `TrackType::folder` + `Track::parent`** — folder tracks: create, name,
  colour, reparent, collapse and expand, mute/solo propagating to children.
  Collapsed state persists with the project. This is where the format bump is
  worth spending.

Definition of done for each: one undo entry, audible or visible through the
existing compiler path, and a test asserting the round-trip through the
project file.

### B5 — clip grouping

Several clips move, copy, delete and colour as one. Groups are project state,
not selection state, and survive save/load.

### B6 — playlist lanes

More than one clip deep on a single track, with overlap resolved by lane
rather than by the last one drawn winning. This changes hit testing and
drawing in `PlaylistView` and the compile order in the graph compiler; expect
it to be the largest of B1–B10.

### B7 — crossfades

Per-clip fade in/out already exists (`Clip::fadeInFrames/fadeOutFrames`). A
crossfade is the paired verb over an overlap: drag one clip onto another and
get complementary fades that stay complementary when either clip is resized.

### B8 — consolidation

Render a selection of clips on a track down to one audio clip, offline,
through the existing `OfflineRender` region path (read it; do not modify it —
it belongs to track C). One undo entry, and the source clips come back.

### B9 — the automation editor

The largest workflow gap in this track. Lanes render in the playlist and
record from the write session, but there is no surface for editing them:
draw and erase points, drag them, set curve type and tension per segment
(`AutomationCurve` already has the vocabulary), box-select, scale a selection
in time and in value, copy and paste between lanes, snap to the grid the
Piano Roll's control strip already publishes.

### B10 — automation clip workflow

"Create automation clip for this parameter" from a mixer or instrument
control's context menu, duplicate, clear, and a way to see which parameter a
lane belongs to without opening it.

### B13 — pattern workflow

Clone, rename, colour, set per-pattern length, and a picker that shows which
patterns are used in the arrangement.

### B11 — multiple arrangements

Several timelines in one project, switched from the toolbar, each with its own
clips and markers, sharing patterns and channels. Format work; sequence it
after B1–B10.

### B12 — Performance Mode (last)

Do not start this until the rest of the track is done, and produce a written
plan before implementing it.

FL Studio's Performance Mode designates the region before the start marker as
a performance zone and triggers clips out of sequence from there, one clip per
track, with per-track press behaviour (retrigger, hold-and-stop,
hold-and-motion, latch), motion behaviour (stay, one-shot, march-and-wrap,
march-and-stay, march-and-stop, random, exclusive random), trigger sync to a
beat interval, and position sync. It is mappable to a pad controller and to
the typing keyboard.

It is engine-deep: the transport currently plays a compiled arrangement from a
position, and this asks it to start and stop clips at quantised boundaries
from a live input, realtime-safe, without recompiling the graph per trigger.
Treat the scheduling design as the deliverable of the first increment.

## Definition of done for this track

Every verb: a command, one undo entry, a keyboard or menu route, project-file
round-trip with a migration where the format moved, and a test that asserts
the behaviour rather than the drawing.

---

## Working rules (identical for all three tracks)

**Your branch.** You work in this worktree, on this branch, and nowhere else.
Push after every commit — `git push origin <this branch>`, **never to `main`**.
The global "push to main after every patch" note does not apply here; main is
where the three tracks get merged, one at a time, by the user.

**Never stage by wildcard.** No `git add -A`, no `git add .`, no `git commit -a`.
Stage the exact paths you changed. Two sessions once shared one working tree in
this project and swept each other's half-written work into unrelated commits;
that is why this rule is written down.

**File boundaries.** Each track owns a disjoint part of the tree (below). Four
files are shared choke points and every track has to touch them:

- `src/CMakeLists.txt`, `tests/CMakeLists.txt` — add your source lines only,
  next to the neighbouring ones. Never reorder or reformat.
- `CHANGELOG.md` — add your entry under `## [Unreleased]`. Do not touch
  released sections.
- `src/ui/macos/main.mm` — the shell. Additive only: your menu item, your
  window, your command registration. Never restructure what is there.

If the work needs a file another track owns, **stop and report it** rather than
editing across the boundary. That is a merge conflict with a person's name on
it, not a code problem.

**The process still applies** (CLAUDE.md §38–§44). The scope in this brief is
pre-approved; individual items inside it are not a licence to skip the rest:

1. Consult the graph first — `graphify query "…"` — before reading files.
2. Smallest coherent increment, then build, then tests.
3. `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug`
   `cmake --build build -j8` — the tree builds with `-Werror`; warnings are errors.
4. `./build/tests/incdaw_tests` — every case green before you commit.
5. New behaviour needs new tests. A DSP block needs a null test at its
   transparent settings and a reference implementation written independently
   of the class under test (see `tests/unit/BuiltinEffectTests.cpp`).
6. Realtime contract: no allocation, no locks, no file or network I/O and no
   unbounded work on the audio thread.
7. `graphify update .` after meaningful source changes, committed with them.
8. A CHANGELOG entry per increment.
9. Project format changes bump `INCDAW_PROJECT_VERSION` and ship a migration —
   an old project must still open.

**Already done for you.** `third_party/clap/clap` is not in git and has been
copied into this worktree; do not delete it and do not commit it.

**Ordering.** The items in your list are ordered. Do them in order unless one
is blocked, and say so if you reorder.
