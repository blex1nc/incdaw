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
