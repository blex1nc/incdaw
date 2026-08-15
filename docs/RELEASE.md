# INCDAW — Release Guide

Status: **Phase 20 — the release process below is live for 0.9.0.**

---

## 1. What a release is

A release is one ad-hoc-signed DMG built by `tools/make-dmg.sh` from a clean
tree whose whole test suite is green in Debug and Release. The version lives
in ONE place — `CMakeLists.txt` `project(VERSION …)` — and is stamped into
the binary (`build/VERSION`, compiled into `app/Version`), the bundle and the
DMG file name at build time, so the artifact can never disagree with the code
about what it is. `src/app/Version.cpp` mirrors it for the UI's status line
and must be bumped in the same commit.

Not notarized, by decision (docs/DECISIONS.md D-009): INCDAW has no
Developer ID enrolment, so the DMG is ad-hoc signed. That is a real
signature — arm64 refuses to execute unsigned binaries — but Gatekeeper will
quarantine any copy that arrives over a network.

---

## 2. Cutting a release

```
# 1. Bump the version (CMakeLists.txt project VERSION + src/app/Version.cpp),
#    add the release section to CHANGELOG.md, commit.
# 2. Verify from clean:
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
(cd build && ctest --output-on-failure)
# 3. Package:
./tools/make-dmg.sh          # -> dist/INCDAW-<version>.dmg
```

The script builds its own Release tree (`build-release/`), signs the bundle
ad hoc, verifies the signature, and produces `dist/INCDAW-<version>.dmg`.

---

## 3. Installing (first launch on another Mac)

1. Copy `INCDAW-<version>.dmg` to the machine and open it.
2. Drag **INCDAW** into **Applications**.
3. First launch — one of the two, once:
   - Right-click (Control-click) **INCDAW.app** → **Open** → **Open**, or
   - `xattr -dr com.apple.quarantine /Applications/INCDAW.app`

   This is required because the app is ad-hoc signed rather than notarized
   (D-009). macOS remembers the choice; subsequent launches are ordinary.
4. Audio: INCDAW opens the default output device at up to 512 frames. A
   Bluetooth output works but cannot sustain small buffers; a wired device
   is the serious choice. Recording needs a microphone-privacy grant on
   first input use.

---

## 4. Updating

Replace `/Applications/INCDAW.app` with the new version (step 3's quarantine
dance applies to each downloaded copy). Projects are forward-compatible by
the format's own rules (docs/PROJECT_FORMAT.md §2): an older project opens
in a newer INCDAW through declared migrations, and a NEWER project is
refused by an older build with a clear message rather than opened hopefully.
Downgrading INCDAW does not downgrade projects — keep the old app around if
you must reopen a project saved by a newer format.

---

## 5. Release notes — 0.9.0 (2026-08-16)

The core is complete: every engineering phase of the roadmap (0–19) plus
this release process. What a musician can do with 0.9.0:

- program patterns in a Metal-rendered Piano Roll and a Channel Rack step
  sequencer; layer them on a playlist into a song
- play the builtin polyphonic synth and a real sampler — multisampled
  zones with velocity layers, sustain loops with crossfades, filter and
  LFO, disk streaming for long samples
- record audio through loop and punch takes into the playlist, edit it in
  the audio editor, stream long clips from disk
- mix on an unlimited mixer with sends, buses, full delay compensation,
  and ten builtin effects (EQ, compressor, limiter, gate, saturator,
  filter, delay, reverb, utility, analyzer) — every parameter automatable
- host CLAP plugins with crash-isolated scanning, opaque state, editors,
  and latency compensation
- automate anything in the parameter registry with curves; bind hardware
  knobs by MIDI learn; both write through the same smoothed setters
- render offline byte-identically to playback — master, stems, tracks or
  regions — to WAV/AIFF at any rate and depth with dithering; exchange
  Standard MIDI Files
- save versioned `.incdaw` packages (format 1.4) that migrate forward
  forever, with plugin state and sample references intact

Known limits, deliberate and recorded: no AU/VST3 yet (CLAP first,
D-007), no FLAC/MP3 (dependency approval pending, §41), no MIDI clock
out (no MIDI output device layer yet), in-process plugin processing
crashes are not survivable (sandboxed hosting is future work), and the
UI is the workshop version — the dedicated UI/UX phase is next.
