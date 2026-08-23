# Dependency proposals — C12, C13, C14

Status: **awaiting the user's decision. Nothing has been added, vendored,
downloaded, or submoduled.**
Written: 2026-08-23, track C ("bridge"). CLAUDE.md §41.

Three gap items need a third-party dependency. Each is set out below in
the form §41 asks for. **C12 is a licensing decision about how INCDAW
itself may be distributed, and is the user's to make, not an engineering
one.**

---

## C12 — VST3 hosting

### The proposal

| | |
|---|---|
| **Name** | Steinberg VST 3 SDK (`vst3sdk`) |
| **Purpose** | Host VST3 plugins — the format most third-party instruments and effects ship in on both macOS and Windows |
| **Licence** | **Dual: GPLv3, or Steinberg's proprietary VST3 licence agreement** |
| **Maintenance** | Actively maintained by Steinberg; the current line is 3.7.x |
| **Platforms** | macOS, Windows, Linux — every platform INCDAW targets or might |
| **Performance** | No runtime cost of its own; it is an interface SDK. Hosting cost is the plugin's |
| **Security** | Same as any plugin host: third-party code in-process. C15 (docs/PLUGIN_SANDBOX_PLAN.md) is the answer to that, and applies to VST3 exactly as to CLAP |

### Why this one

There is no alternative that hosts VST3. The format is defined by this
SDK; a clean-room implementation of the interfaces would be a
reverse-engineering project with legal exposure worse than either licence.

### The decision, and why it is yours

The two licences are not interchangeable:

- **GPLv3** — INCDAW may use the SDK freely, but INCDAW itself must then
  be distributed under GPLv3 (or a compatible licence), with source. That
  forecloses ever shipping INCDAW as a closed-source product.
- **Steinberg's proprietary agreement** — INCDAW may stay closed-source.
  It requires registering as a VST3 licensee, accepting Steinberg's terms
  (including logo/trademark usage rules), and is granted rather than
  merely downloaded.

Which one is right depends entirely on how you intend to distribute
INCDAW, which is not a question the code can answer. **I have not chosen,
and will not.**

### If neither is acceptable

CLAP already hosts (and is MIT-licensed, no strings), AU hosts on macOS
including instruments as of C16. A DAW that hosts CLAP + AU on macOS is a
usable DAW; VST3 is about the breadth of the third-party catalogue, not
about whether plugins work at all.

---

## C13 — FLAC import and export

### The proposal

| | |
|---|---|
| **Name** | libFLAC (the reference implementation, from Xiph.Org) |
| **Purpose** | Read and write FLAC — lossless, roughly half the size of WAV, and what a great many sample libraries and archives ship in |
| **Licence** | **BSD 3-clause** for the library; the tools are GPL but are not needed. No effect on how INCDAW may be distributed |
| **Maintenance** | Xiph.Org, continuously maintained since 2000; current line 1.4.x |
| **Platforms** | Everywhere. Plain C89, no platform assumptions |
| **Performance** | Decode is fast enough for realtime streaming; encode is offline in INCDAW's use (export and bounce), so its cost is not on any hot path. Both allocate, which is fine — `WavFile` is already explicitly non-realtime asset I/O |
| **Security** | It parses untrusted files, which is the risk. It is also one of the most-fuzzed audio codecs in existence, precisely because everything uses it. The reader must still refuse a malformed file rather than trust it — the same contract this track's definition of done already states |

### Alternatives

- **`dr_flac`** (single-header, public domain) — decode only, and no
  encoder. Half the feature, so it does not answer C13.
- **macOS `AudioToolbox`** — decodes FLAC on recent macOS versions but
  does not encode it, and ties the feature to one platform.
- **Write our own** — a FLAC encoder is weeks of work to arrive somewhere
  worse than a BSD-licensed reference implementation.

### Why this one

It is the format's reference implementation, it is BSD, it encodes as
well as decodes, and it is portable. There is no case for anything else.

### Integration shape (if approved)

`FlacFile` beside `WavFile`, same `AudioFileData` in and out, same
`Result` shape. Vendored under `third_party/flac` or fetched by CMake —
whichever you prefer; the code does not care.

---

## C14 — Compressed export

Two options, and I recommend the second.

### Option A — MP3 via LAME

| | |
|---|---|
| **Name** | LAME (`libmp3lame`) |
| **Purpose** | Encode MP3 |
| **Licence** | **LGPL 2.1.** Usable from a closed-source application if linked dynamically (or if object files are shipped for relinking); static linking into a closed binary is the case to avoid |
| **Maintenance** | Alive but slow — the last release was some years ago. It is a mature codec, so that is not alarming in itself |
| **Platforms** | Everywhere |
| **Performance** | Offline encode; not on any hot path |
| **Security** | Encoder only in this use; it is fed our own float buffers, not untrusted input |
| **Patents** | The MP3 patents expired in 2017. This is stated because it is the first question anyone asks, and the answer is that it is no longer a concern |

The real cost is not legal, it is that LGPL dynamic linking constrains how
INCDAW is packaged (a `.dylib` inside the bundle, relinkable), and that is
a build and notarisation detail to get right.

### Option B — AAC and ALAC via macOS AudioToolbox (recommended)

| | |
|---|---|
| **Name** | `AudioConverter` / `AudioFile`, part of AudioToolbox |
| **Purpose** | Encode AAC (lossy, `.m4a`) and ALAC (lossless, `.m4a`) |
| **Licence** | **None to accept — it is the operating system.** No dependency is added at all |
| **Maintenance** | Apple's |
| **Platforms** | macOS only. Windows would need a separate encoder later, and that is a real limitation |
| **Performance** | Hardware-accelerated where available; offline in this use |
| **Security** | System framework, fed our own buffers |

### Why I recommend B

The brief frames C14 as "a compressed file to send someone", and for that
purpose AAC at 256 kbps is better than MP3 at 320 and `.m4a` plays
everywhere a person would send it. It adds **no dependency**, no licence to
accept, and no packaging constraint — and INCDAW is macOS-first today, so
the platform limitation costs nothing now.

ALAC comes free with the same API, which also gives a lossless compressed
format without waiting on the C13 decision.

**MP3 remains worth adding later** — it is the format people ask for by
name — but it should be a deliberate second step taken when Windows
support forces a portable encoder anyway, not the first thing built.

### If B is chosen

`AacFile` in `src/platform/macos/`, behind an interface in
`src/engine/audio/` so a portable encoder can be added later without
changing a caller. No `third_party` change, no CMake dependency, no
licence file.

---

## What happens next

Nothing, until you say so. For each item the answer needed is:

- **C12** — GPLv3, proprietary agreement, or not yet.
- **C13** — yes or no to libFLAC (BSD, and I recommend yes).
- **C14** — Option A, Option B, both, or neither. **I recommend B.**
