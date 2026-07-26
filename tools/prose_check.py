#!/usr/bin/env python3
"""Monorepo prose guard.

Enforces the typography rules of AGENTS.md over every file the project owns: documentation,
build and tooling files, and the text the code itself emits. The em dash is banned outright, with
no exception and therefore no allowlist (AGENTS.md 5). Vendored and CubeMX-generated trees are not
project-owned and are out of scope.

Usage:
    python3 tools/prose_check.py [--repo-root PATH]
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

# Written as an escape so this file passes its own check; a literal would be the first violation.
EM_DASH = "\u2014"

# Trees this repository does not own: vendored code and everything CubeMX regenerates
# (AGENTS.md 1.2, firmwares/AGENTS.md F.1).
EXCLUDED_PATH_SEGMENTS = (
    "third_party/",
    "/Core/",
    "/Drivers/",
    "/Middlewares/",
    "/USB_DEVICE/",
    "/Third_Party/",
)

# Files whose bytes are not prose: binaries, lockfiles and generated hardware descriptions.
EXCLUDED_SUFFIXES = (
    ".bin",
    ".elf",
    ".ico",
    ".ioc",
    ".jpg",
    ".jpeg",
    ".lock",
    ".pdf",
    ".png",
    ".svd",
    ".ttf",
    ".webp",
)


@dataclass(frozen=True)
class Violation:
    check: str
    path: str
    line: int
    column: int
    message: str

    def render(self) -> str:
        return f"{self.path}:{self.line}:{self.column}: [{self.check}] {self.message}"


def IsProjectOwned(repo_root: Path, relative_path: str) -> bool:
    if any(segment in f"/{relative_path}" for segment in EXCLUDED_PATH_SEGMENTS):
        return False
    if relative_path.endswith(EXCLUDED_SUFFIXES):
        return False
    # A symlink carries no text of its own; reading it would report its target twice, once under
    # each name (CLAUDE.md and AGENTS.md are the same file).
    return not (repo_root / relative_path).is_symlink()


def CollectTrackedFiles(repo_root: Path) -> list[str]:
    """The tracked set is the honest definition of what the project owns: it already excludes
    build outputs, the scratch directory and anything else .gitignore keeps out."""
    listing = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=repo_root,
        capture_output=True,
        text=True,
    )
    if listing.returncode != 0:
        raise SystemExit(
            f"prose_check needs the tracked file list and git refused it:\n{listing.stderr.strip()}\n"
            "In a container, add the workspace to safe.directory first."
        )
    return sorted(
        relative_path
        for relative_path in listing.stdout.split("\0")
        if relative_path and IsProjectOwned(repo_root, relative_path)
    )


def CheckEmDash(repo_root: Path, relative_path: str) -> list[Violation]:
    try:
        text = (repo_root / relative_path).read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []

    return [
        Violation(
            check="em-dash",
            path=relative_path,
            line=line_number,
            column=line.index(EM_DASH) + 1,
            message=(
                "em dash is banned; use a semicolon to join two clauses, "
                "a colon to introduce, a comma or parentheses to interrupt, "
                "or simply two sentences"
            ),
        )
        for line_number, line in enumerate(text.splitlines(), start=1)
        if EM_DASH in line
    ]


def Main() -> int:
    parser = argparse.ArgumentParser(description="Check monorepo prose rules.")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parent.parent)
    arguments = parser.parse_args()
    repo_root = arguments.repo_root.resolve()

    tracked_files = CollectTrackedFiles(repo_root)
    violations = []
    for relative_path in tracked_files:
        violations.extend(CheckEmDash(repo_root, relative_path))

    for violation in violations:
        print(violation.render())

    if violations:
        print(f"\nProse check: {len(violations)} violation(s) over {len(tracked_files)} files.")
        return 1
    print(f"Prose check: clean over {len(tracked_files)} files.")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
