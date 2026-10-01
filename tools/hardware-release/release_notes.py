#!/usr/bin/env python3
"""Write the board section of a hardware release's notes.

Hardware releases go through release-please, like firmware (design decision D10; the
standard is the "Releases" section of ``docs/_source/hardware-standards.md``): merging the
hardware release PR tags ``hardware-vX.Y.Z`` and publishes a GitHub Release whose notes are
release-please's changelog of ``PCB/``. This script writes what that changelog can't —
which boards the release contains, at which version, and what changed in each — from
``PCB/manifest.yaml``:

  * minimum firmware, and a table of every board: version, and whether it is new,
    unchanged, or moved since the previous hardware release;
  * per board: fab order ID, errata, the commit the boards were made from, and git-cliff's
    list (``cliff.toml``) of the commits that touched the board folder up to that commit —
    since the board's previous release, or since release-please's ``bootstrap-sha`` for a
    board's first release.

Each board's title-block revision is checked at the commit it was made from.

Usage::

    release_notes.py hardware-vX.Y.Z     # after the release: notes for that tag
    release_notes.py --preview           # before it: notes for the manifest at HEAD

Prints the markdown on stdout. Exit code 0 on success, 1 on any problem.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

import yaml

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
MANIFEST = "PCB/manifest.yaml"
TAG_RE = re.compile(r"^hardware-v(\d+)\.(\d+)\.(\d+)$")


class ReleaseError(Exception):
    pass


def git(*args: str) -> str:
    return subprocess.run(
        ["git", *args], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout


def git_ok(*args: str) -> bool:
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True).returncode == 0


def version_key(tag: str) -> tuple[int, ...]:
    return tuple(int(part) for part in TAG_RE.match(tag).groups())


def previous_release(tag: str | None) -> str | None:
    """The hardware release just below ``tag`` (or the newest one, for a preview)."""
    tags = [t for t in git("tag", "-l", "hardware-v*").split() if TAG_RE.match(t)]
    if tag:
        tags = [t for t in tags if version_key(t) < version_key(tag)]
    return max(tags, key=version_key) if tags else None


def bootstrap_sha() -> str:
    config = json.loads((ROOT / "release-please-config.json").read_text(encoding="utf-8"))
    return config["bootstrap-sha"]


def load_manifest(ref: str) -> dict:
    try:
        return yaml.safe_load(git("show", f"{ref}:{MANIFEST}")) or {}
    except subprocess.CalledProcessError:
        return {}  # before the manifest existed


def short(sha: str) -> str:
    return git("rev-parse", "--short", sha).strip()


def board_section(entry: dict, before: dict | None, start: str, ref: str, release: str) -> str:
    board, path, version = entry["board"], entry["path"], str(entry["version"])
    fabricated = str(entry["fabricated"])
    if not git_ok("cat-file", "-e", f"{fabricated}^{{commit}}"):
        raise ReleaseError(f"{board}: fabricated commit {fabricated} doesn't exist")
    sch = f"{path}/{board}.kicad_sch"
    if f'(rev "{version}")' not in git("show", f"{fabricated}:{sch}"):
        raise ReleaseError(f"{board}: title-block revision at {short(fabricated)} is not {version}")

    lines = [f"## {board} {version}", ""]
    facts = [f"Made from `{short(fabricated)}`"]
    if entry.get("order"):
        facts.insert(0, f"Order {entry['order']}")
    lines.append(" · ".join(facts))
    if entry.get("errata"):
        lines += ["", f"**Errata:** {entry['errata']}"]
    if not git_ok("diff", "--quiet", fabricated, ref, "--", path, f":!{path}/README.md"):
        lines += ["", f"The design has moved on since; this revision's files are at `{short(fabricated)}`."]
    lines.append("")

    if before and str(before["version"]) == version:
        lines.append("Unchanged since the previous hardware release.")
        return "\n".join(lines)

    since = str(before["fabricated"]) if before else start
    changes = subprocess.run(
        ["git-cliff", "--config", str(ROOT / "cliff.toml"), "--strip", "all",
         "--include-path", f"{path}/**", "--tag-pattern", "^hardware-v",
         "--tag", release, f"{since}..{fabricated}"],
        cwd=ROOT, check=True, capture_output=True, text=True,
    ).stdout.strip()
    lines.append(changes)
    return "\n".join(lines)


def notes(tag: str | None) -> str:
    if tag and not git_ok("rev-parse", "-q", "--verify", f"refs/tags/{tag}"):
        raise ReleaseError(f"{tag} does not exist (use --preview before a release)")
    ref = tag or "HEAD"
    # A preview reads the working copy, so uncommitted manifest edits show up.
    manifest = load_manifest(ref) if tag else (
        yaml.safe_load((ROOT / MANIFEST).read_text(encoding="utf-8")) or {})
    boards = list(manifest.get("boards") or [])
    if not boards:
        raise ReleaseError(f"{MANIFEST} lists no boards")

    prev = previous_release(tag)
    before = {b["board"]: b for b in (load_manifest(prev).get("boards") or [])} if prev else {}
    start = prev or bootstrap_sha()
    release = tag or f"hardware-v{manifest.get('release', '?')}"

    rows = []
    for entry in boards:
        was = before.get(entry["board"])
        version = str(entry["version"])
        if was is None:
            change = "new"
        elif str(was["version"]) == version:
            change = "unchanged"
        else:
            change = f"{was['version']} → {version}"
        rows.append(f"| {entry['board']} | {version} | {change} |")

    out = [
        "## Boards",
        "",
        f"**Minimum firmware:** {manifest.get('min_firmware') or 'not set'}",
        "",
        "| Board | Version | Change |",
        "|---|---|---|",
        *rows,
    ]
    for entry in boards:
        out += ["", board_section(entry, before.get(entry["board"]), start, ref, release)]
    return "\n".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("tag", nargs="?", help="a published hardware-vX.Y.Z tag")
    group.add_argument("--preview", action="store_true",
                       help="notes for the manifest at HEAD, before the release")
    args = parser.parse_args()

    if args.tag and not TAG_RE.match(args.tag):
        print(f"error: {args.tag} is not a hardware release tag", file=sys.stderr)
        return 1
    try:
        print(notes(None if args.preview else args.tag))
    except (ReleaseError, KeyError) as err:
        print(f"error: {err}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
