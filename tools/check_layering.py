#!/usr/bin/env python3
"""Static enforcement of INCDAW's layer boundaries.

Implements the rules in docs/ARCHITECTURE.md §2. Run as part of the test suite
so that an architectural violation fails the build exactly like a failing
assertion does.

Two rules are checked:

1. DEPENDENCY DIRECTION — dependencies point downward only. A layer may include
   headers from its own layer or any layer below it, never above.

2. PLATFORM CONTAINMENT — operating-system APIs appear only in `platform/` and
   `ui/`. The engine, project, app and plugin-core layers must compile with no
   knowledge of which OS they are on, because that is what makes them testable
   headlessly and portable later (docs/DECISIONS.md D-005).

Usage:  check_layering.py <src-dir>
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

# Lower number = lower layer. A layer may depend on anything with a level <= its own.
LAYERS: dict[str, int] = {
    "platform": 0,
    "engine": 1,
    "plugins": 1,
    "project": 2,
    "app": 3,
    "ui": 4,
}

# Layers permitted to touch operating-system APIs directly.
#   platform/ — by definition, this is where OS code lives.
#   ui/       — the native shell is inherently AppKit/Metal.
PLATFORM_ALLOWED = {"platform", "ui"}

# Headers and module prefixes that mean "this file knows what OS it is on".
OS_HEADER_PATTERNS = [
    r"^Cocoa/", r"^AppKit/", r"^Foundation/", r"^CoreFoundation/",
    r"^CoreAudio/", r"^AudioToolbox/", r"^AudioUnit/", r"^CoreMIDI/",
    r"^AVFoundation/", r"^AVFAudio/", r"^CoreAudioKit/",
    r"^Metal/", r"^MetalKit/", r"^QuartzCore/", r"^Accelerate/",
    r"^mach/", r"^objc/", r"^os/",
    r"^sys/sysctl\.h$", r"^sys/mman\.h$",
    r"^pthread\.h$", r"^unistd\.h$",
    r"^windows\.h$", r"^Windows\.h$",
]
OS_HEADER_RE = re.compile("|".join(OS_HEADER_PATTERNS))

INCLUDE_RE = re.compile(r'^\s*#\s*(?:include|import)\s*[<"]([^>"]+)[>"]')

SOURCE_SUFFIXES = {".h", ".hpp", ".c", ".cc", ".cpp", ".m", ".mm"}


def layer_of(path: Path, src_root: Path) -> str | None:
    try:
        first = path.relative_to(src_root).parts[0]
    except ValueError:
        return None
    return first if first in LAYERS else None


def check(src_root: Path) -> list[str]:
    violations: list[str] = []

    for path in sorted(src_root.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES or not path.is_file():
            continue

        layer = layer_of(path, src_root)
        if layer is None:
            continue

        level = LAYERS[layer]
        rel = path.relative_to(src_root.parent)

        for number, line in enumerate(path.read_text(errors="replace").splitlines(), start=1):
            match = INCLUDE_RE.match(line)
            if not match:
                continue

            included = match.group(1)

            # Rule 1 — dependency direction.
            included_layer = included.split("/")[0]
            if included_layer in LAYERS and LAYERS[included_layer] > level:
                violations.append(
                    f"{rel}:{number}: '{layer}' includes '{included}' from the higher "
                    f"layer '{included_layer}'. Dependencies point downward only."
                )

            # Rule 2 — platform containment.
            if layer not in PLATFORM_ALLOWED and OS_HEADER_RE.match(included):
                violations.append(
                    f"{rel}:{number}: '{layer}' includes the OS header '{included}'. "
                    f"Operating-system APIs belong in platform/."
                )

    return violations


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} <src-dir>", file=sys.stderr)
        return 2

    src_root = Path(sys.argv[1]).resolve()
    if not src_root.is_dir():
        print(f"error: '{src_root}' is not a directory", file=sys.stderr)
        return 2

    violations = check(src_root)

    if violations:
        print(f"Layering check FAILED — {len(violations)} violation(s):\n", file=sys.stderr)
        for violation in violations:
            print(f"  {violation}", file=sys.stderr)
        print("\nSee docs/ARCHITECTURE.md §2.", file=sys.stderr)
        return 1

    print(f"Layering check passed: {', '.join(sorted(LAYERS))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
