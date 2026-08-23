# INCDAW — Project Format

Status: **Phase 0 — design only. Not implemented.**
Target: Phase 4.

    INCDAW_PROJECT_VERSION = 1.0

CLAUDE.md §2 is absolute: **never create an unversioned project format.** The
version is written from the very first project file INCDAW ever saves.

---

## 1. Shape: a package directory, not a single file

A project is a directory with the `.incdaw` extension, presented as a single
document in Finder (macOS package bundle).

```
Song.incdaw/
├── manifest.json          format version, app version, timestamps, checksums
├── project.json           tracks, channels, mixer, routing, markers
├── patterns/              MIDI + automation events (binary)
│   ├── 0001.pat
│   └── 0002.pat
├── automation/            automation lanes (binary)
├── plugins/               opaque per-instance plugin state blobs
│   └── <instance-id>.state
├── media/                 embedded audio, or absent when referenced
└── history/               autosave snapshots + crash recovery
```

**Why a directory rather than one opaque file:**

| Benefit | Consequence |
|---|---|
| Partial save | Only changed patterns rewrite; large projects save fast |
| Streaming load | Audio and plugin state load lazily; project opens quickly |
| Crash resilience | A corrupted pattern loses one pattern, not the session |
| Version control | Text manifest diffs; binaries are separate files |
| Recovery | `history/` survives independently of the main project |

Trade-off: more files on disk, and a packaging step for archiving/transfer
(§6). Accepted — the resilience is worth it for a document users spend months in.

---

## 2. Versioning and migration

`manifest.json` is the first thing read and is **always** plain, minimal JSON so
that any future version can read it, whatever else changes:

```json
{
  "incdaw_project_version": "1.0",
  "created_with": "INCDAW 0.1.0",
  "last_saved_with": "INCDAW 0.1.0",
  "created": "2026-08-14T15:00:00Z",
  "modified": "2026-08-14T15:00:00Z"
}
```

**Migration rules:**

- Migrations form a chain: `v1.0 → v1.1 → v2.0`. There is never a direct
  `v1.0 → v3.0` path to maintain.
- Migration is **one-way and non-destructive**: the original is backed up into
  `history/` before any migration runs.
- Opening a project saved by a *newer* INCDAW is refused with a clear message,
  never attempted optimistically.
- **Every released format version gets a permanent test fixture** in
  `tests/fixtures/`, and every one of them must keep loading forever. A
  migration that breaks an old fixture fails CI.

Minor version = additive, backward-compatible. Major version = breaking,
requires a migration step.

### Version history

| Version | Phase | Change |
|---|---|---|
| 1.0 | 4 | First format. |
| 1.1 | 8 | Pattern notes stored per channel (`channels[]` in a `.pat` file) instead of one flat `events[]`; patterns gained `swing` / `swingGrid`; clips gained `startTick` / `lengthTicks` / `sourceOffsetTicks`. |
| 1.2 | 8b | Channels gained `stepKey`, the pitch a step sequencer step is written at. Additive. |
| 1.3 | 14 | Channels gained `samplerZones[]`, the builtin sampler's program — each zone names an audio asset by id plus key/velocity range, slice, sustain loop, crossfade, reverse and gain. Additive. |
| 1.4 | 16 | The project gained `midiMappings[]` — hardware controls bound to parameters by registry key and target entity, with a normalised (possibly inverted) output range. Additive. |
| 1.5 | UI build-out 4 | Channels gained `instrumentParameters[]` — stored instrument parameter values ({id, value}, plain units), applied through the instrument's sink at every compile (D-034). Additive. |
| 1.6 | FL2026 P3/P5 | The project gained `markers[]` — named musical positions with an optional length — and mixer nodes gained `stereoWidth`. Additive. |
| 1.7 | TRACK B / B4 | Tracks gained `collapsed`, whether a folder track hides its children. `type` has always serialized as an integer and `parent` has always been written, so folder tracks themselves need no conversion. Additive. |
| 1.8 | TRACK B / B5 | Clips gained `group`, the clip group they belong to. A group is exactly the set of clips carrying one id; it names no entity of its own. Additive. |
| 1.9 | TRACK B / B6 | Clips gained `lane`, which lane of their track they occupy. A track's lane count is derived from its clips rather than stored, so nothing else moved. Additive. |
| 1.10 | TRACK B / B7 | Clips gained `crossfadeIn` / `crossfadeOut`, whether each edge crossfades with the clip it overlaps on its lane. Per edge, so three chained clips can keep one crossfade and lose the other. The fade lengths stay derived from the overlap. Additive. |
| 1.11 | TRACK B / B11 | The project gained `arrangements[]` and `currentArrangement`. Clips and markers moved from the top level into an arrangement; patterns, channels, tracks, the mixer and automation stay shared. **Shape change**, converted at the read site. |
| 1.12 | TRACK B / B12 | Tracks gained their Performance Mode press, motion, trigger sync and position sync; clips gained `performanceKey`, the pad that triggers them; markers gained `isStart`, which makes the region before one a performance zone. Additive. |
| 1.13 | TRACK B / B12 | A MIDI mapping gained `kind` and, for a pad mapping, `performancePad`. A pad mapping reads `controller` as a note number rather than a CC. Additive. |

**Reading a 1.0 file.** Two of the three changes need context the migration hook
does not have — the pattern files are separate documents, and converting clip
frames to ticks needs the tempo map — so those upgrades happen at their read
sites, which know the version being read. `ProjectFile::migrate` remains the
single place that decides whether a path from a given version exists at all.

A 1.0 pattern's flat event list is read into a content block that names no
channel, and is attached to the project's first channel once the id generator
has been restored — creating a channel if the project has none. Notes that
loaded but belonged to no channel would be present in the model and silent on
playback, which is worse than not loading them.

**Reading a 1.1 file.** Purely additive: a 1.1 document has no `stepKey` and the
reader defaults it to 60, which is the pitch those projects behaved as if they
had. Nothing is rewritten, but the path is still declared in
`ProjectFile::migrate` — an undeclared version is refused, because silently
accepting unknown ones is how a loader starts dropping fields.

`tests/fixtures/v1.1/Fixture.incdaw` is kept permanently, alongside the 1.0 one.
It is a frozen document rather than something the current writer regenerates:
two channels, one pattern with per-channel content and swing, and no `stepKey`
anywhere.

**Reading a 1.2 file.** Purely additive again: a 1.2 document has no
`samplerZones` and the reader defaults each channel to an empty program, which
is what those channels had. The path is declared in `ProjectFile::migrate`.

`tests/fixtures/v1.2/Fixture.incdaw` (frozen: `stepKey` present, no
`samplerZones`) and `tests/fixtures/v1.3/Fixture.incdaw` (frozen: a channel
whose `instrument` is `builtin:incdaw.sampler` with one zone naming an audio
asset) are kept permanently beside the earlier ones.

**Reading a 1.3 file.** Purely additive: no `midiMappings`, read back empty.
`tests/fixtures/v1.4/Fixture.incdaw` (frozen: one mapping, CC 74 to the master
node's volume) is kept permanently.

**Reading a 1.4 file.** Purely additive: no `instrumentParameters`, read back
empty — those instruments played at their defaults and still do.
`tests/fixtures/v1.5/Fixture.incdaw` (frozen: a sampler channel carrying two
stored parameter values) is kept permanently.

**Reading a 1.5 file.** Purely additive: no `markers`, read back empty, and a
mixer node without `stereoWidth` reads back at unity.
`tests/fixtures/v1.6/Fixture.incdaw` (frozen: one marker and one region) is
kept permanently.

**Reading a 1.6 file.** Purely additive: no `collapsed`, read back false —
every folder open, which is how those projects were last drawn, because 1.6
had no way to close one. The grouping itself is not new: a 1.6 file that named
a folder in a track's `parent` gets that folder's mute and solo propagation
under 1.7 without any conversion. `tests/fixtures/v1.7/Fixture.incdaw` (frozen:
a collapsed folder holding an instrument track and an audio track) is kept
permanently.

**Reading a 1.7 file.** Purely additive: no `group`, read back invalid — every
clip on its own, which is what those projects had, since 1.7 had no way to
group one. `tests/fixtures/v1.8/Fixture.incdaw` (frozen: three pattern clips,
two of them grouped) is kept permanently.

**Reading a 1.8 file.** Purely additive: no `lane`, read back zero — one lane
per track, which is all those projects ever had.
`tests/fixtures/v1.9/Fixture.incdaw` (frozen: two pattern clips sharing a span
on two lanes of one track) is kept permanently.

**Reading a 1.9 file.** Purely additive: neither crossfade flag, read back
false — those clips play the manual fades they always did.
`tests/fixtures/v1.10/Fixture.incdaw` (frozen: two audio clips overlapping by
1,000 frames with the pair crossfaded) is kept permanently.

**Reading a 1.10 file.** The one shape change since 1.1. A 1.10 document has
`clips` and `markers` at the top level, because a 1.10 project was one
timeline; the reader wraps them into a single arrangement named
"Arrangement 1" and makes it current. Converted at the read site rather than
in `migrate`, for the same reason 1.0's clip timing was: it needs the tempo
map, which exists only once the document has been read that far. The
arrangement is minted at the document's `nextEntityId` and the generator is
told about it, or the next entity created in the session would be handed the
same number. `tests/fixtures/v1.11/Fixture.incdaw` (frozen: two arrangements,
a marker on the first and the second current) is kept permanently.

**Reading a 1.11 file.** Purely additive: no track has performance settings, no
clip has a pad and no marker is a start marker, which reads back as a project
with no performance zone — exactly what those projects were.
`tests/fixtures/v1.12/Fixture.incdaw` (frozen: a latching, marching performance
track of two pads with a start marker at bar 5) is kept permanently.

**Reading a 1.12 file.** Purely additive: a mapping with no `kind` is a
parameter mapping, which is what every mapping written since 1.4 was.
`tests/fixtures/v1.13/Fixture.incdaw` (frozen: one CC mapping and two pad
mappings on MIDI channel 10) is kept permanently.

---

## 3. Text vs binary

| Data | Encoding in v1.0 | Reason |
|---|---|---|
| `manifest.json`, `project.json` | JSON (UTF-8) | Human-readable, diffable, inspectable, debuggable |
| Patterns (`patterns/*.pat`) | JSON, one file per pattern | See below |
| Plugin state | Opaque binary | Owned by the plugin; INCDAW never interprets it |
| Media | Native audio format | No transcoding on save |

**Why patterns are JSON in v1.0, not binary.** The original plan called for a
binary pattern encoding on the grounds that JSON would be too slow for millions
of events. That is a real concern eventually, but it is not one that has been
measured, and docs/PERFORMANCE.md §4 forbids optimising ahead of measurement.

What v1.0 does commit to is the part that makes the change cheap later: each
pattern is **its own file**, indexed from `project.json` by id and filename.
The encoding of an individual pattern file can therefore change — to binary, or
to anything else — without touching `project.json`, without a format major
version bump for the rest of the document, and without invalidating any other
file in the package. The decision is deferred, not foreclosed.

INCDAW's own JSON writer is used rather than a library, because the determinism
requirement in §7 rules out any implementation that iterates object keys in hash
order.

---

## 4. Media: referenced or embedded

Each `AudioAsset` records both a path and a content hash.

- **Referenced** (default): the file stays where the user put it. The project
  stores a relative path when the file is inside the project directory, and an
  absolute path plus hash otherwise.
- **Embedded**: the file is copied into `media/`. Used for recordings, and when
  the user explicitly consolidates.

**Missing media handling** is designed in from v1.0, not added later:

1. On load, every referenced asset is verified by path, then by hash.
2. Missing assets are reported in a single dialog — never one at a time.
3. Relinking searches the project directory, then user-configured search paths,
   then matches by content hash, then asks the user.
4. A project with missing media **still opens**, with placeholder assets that
   preserve all clip positions, gains, and edits. Nothing is discarded.

---

## 5. Autosave, backup and recovery

- Autosave writes a snapshot into `history/` on an interval and before risky
  operations (migration, plugin scan, bounce).
- Snapshots are rotated with a configurable retention count.
- Backup filenames include the save date — FL Studio 2026 does the same, and it
  is the difference between a usable backup folder and an unusable one.
- On abnormal termination, the next launch offers the most recent snapshot.
- Autosave never overwrites the user's own save; it is always a separate file.

---

## 6. Archiving

"Package project" produces a single `.incdaw-archive` (zip) containing the
package directory with **all referenced media embedded** and a manifest of
required plugins with their identifiers and versions. Opening an archive that
requires missing plugins lists exactly what is needed rather than failing
opaquely.

---

## 7. Determinism

Saving an unmodified project twice must produce byte-identical output. This
means:

- stable ordering of all collections (by id, never by hash-map iteration order);
- no embedded timestamps outside `manifest.json`;
- no absolute paths where a relative path is possible.

Determinism is what makes the round-trip regression test meaningful, and what
makes the project format usable with version control.

---

## 8. Tests (Phase 4 gate)

1. **Round-trip:** save → load → in-memory model is identical.
2. **Determinism:** save → save → byte-identical files.
3. **Version fixtures:** every historical version still loads.
4. **Missing media:** project opens, edits preserved, relink succeeds.
5. **Corruption:** truncated and malformed files are rejected cleanly, never
   crash, never silently lose data.
6. **Fuzz:** randomly corrupted project files must not crash the loader.

---

## 9. Files that are not the project

A project stores what the music is. Two other formats store what the *machine*
is, and they live in `~/Library/Application Support/INCDAW` rather than in the
package — because putting either in the project is what makes a project
unopenable on a second Mac (docs/DECISIONS.md D-036).

Both are versioned, both tolerate unknown and missing keys, and both degrade to
defaults rather than to an error: they are caches of a preference, never
preconditions for launching.

### `settings.json` — `app::AppSettings`

    { "format": "incdaw-settings", "version": 1, … }

The audio device, rate and block size; MIDI sources; the window's last frame
and editor; the update check; and `appearance.theme`, the name of the scheme
the shell draws with.

### `Themes/<name>.json` — `ui::theme::ThemePalette`

    { "format": "incdaw-theme", "version": 1, "name": …,
      "colours": { "windowBackground": "#FF0F1115", … } }

One file per user theme, one `#AARRGGBB` string per role (D-039). The alpha
byte is always written: the two bevel roles exist only as a partial alpha and
would draw as solid without it.

**The file name is the theme's name**, not the `name` field — a theme travels
between machines by being copied and renamed, so the folder is the authority.
The built-in schemes are compiled in and are never written here; a user theme
may not take a built-in's name.

A colour that does not parse, a role this build does not have, or a file that
is not JSON at all costs that role its value and nothing else.
