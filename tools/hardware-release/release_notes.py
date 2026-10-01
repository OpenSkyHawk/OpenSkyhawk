#!/usr/bin/env python3
"""Write the GitHub Release notes for a hardware release tag.

Hardware release track: design decision D10; the standard is the "Releases" section of
``docs/_source/hardware-standards.md``. Two kinds of tag:

  * ``pcb/<Board>-vX.Y.Z`` — one board, released after bring-up. Notes = the tag message
    (fab order ID, any errata) + git-cliff's list of commits that touched the board's
    folder since the board's previous tag.
  * ``hardware-vX.Y.Z`` — the cockpit. Notes = minimum firmware, a table of every board in
    ``hardware/manifest.yaml`` (version, and what moved since the previous cockpit
    release), then each board's notes.

``-v0.0.0`` tags are baselines changelogs count from; they get no release.

git-cliff does the commit parsing (``cliff.toml``). The range and version are passed to it
explicitly, because a tag on a commit outside the board folder is invisible to a
path-filtered git-cliff.

Usage::

    release_notes.py TAG [--dry-run] [--notes FILE]

Prints the release title on stdout and writes the notes to FILE (default: stdout after the
title). ``--dry-run`` previews a tag that doesn't exist yet: the range ends at HEAD.
Exit code 0 on success (or a baseline tag, which prints nothing), 1 on any problem.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys

import yaml

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
MANIFEST = "hardware/manifest.yaml"
TAG_RE = re.compile(r"^(?:pcb/(?P<board>.+)|hardware)-v(?P<version>\d+\.\d+\.\d+)$")


class ReleaseError(Exception):
    pass


def git(*args: str) -> str:
    return subprocess.run(
        ["git", *args], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout


def tag_exists(tag: str) -> bool:
    return (
        subprocess.run(
            ["git", "rev-parse", "-q", "--verify", f"refs/tags/{tag}"],
            cwd=ROOT, capture_output=True,
        ).returncode
        == 0
    )


def version_key(tag: str) -> tuple[int, ...]:
    return tuple(int(part) for part in tag.rsplit("-v", 1)[1].split("."))


def previous_tag(prefix: str, tag: str) -> str | None:
    """The tag just below ``tag`` among the tags starting with ``prefix``."""
    lower = [
        t for t in git("tag", "-l", f"{prefix}*").split()
        if TAG_RE.match(t) and version_key(t) < version_key(tag)
    ]
    return max(lower, key=version_key) if lower else None


def board_folder(board: str) -> pathlib.Path:
    found = [
        p for p in git("ls-files", "PCB").splitlines()
        if p.endswith(f"/{board}/{board}.kicad_pro")
    ]
    if len(found) != 1:
        raise ReleaseError(f"expected one KiCad project named {board} under PCB/, found {len(found)}")
    return pathlib.PurePosixPath(found[0]).parent


def check_revision(board: str, folder: pathlib.PurePosixPath, version: str) -> None:
    """The tagged tree must be the board that was built: its title block carries the version."""
    sch = ROOT / folder / f"{board}.kicad_sch"
    if f'(rev "{version}")' not in sch.read_text(encoding="utf-8"):
        raise ReleaseError(
            f"{folder}/{board}.kicad_sch title-block revision is not {version} "
            "— is the tag on the as-fabricated commit?"
        )


def board_notes(board: str, folder: pathlib.PurePosixPath, tag: str) -> str:
    parts = []
    message = git("tag", "-l", "--format=%(contents)", tag).strip()
    if message:
        parts.append(message)
    prev = previous_tag(f"pcb/{board}-v", tag)
    if prev is None:
        parts.append("Initial tagged revision.")
    else:
        end = tag if tag_exists(tag) else "HEAD"
        changes = subprocess.run(
            ["git-cliff", "--config", str(ROOT / "cliff.toml"), "--strip", "all",
             "--include-path", f"{folder}/**", "--tag-pattern", f"^pcb/{board}-v",
             "--tag", tag, f"{prev}..{end}"],
            cwd=ROOT, check=True, capture_output=True, text=True,
        ).stdout.strip()
        parts.append(changes)
    return "\n\n".join(parts)


def manifest_boards(text: str) -> tuple[str, list[dict]]:
    data = yaml.safe_load(text) or {}
    return str(data.get("min_firmware") or ""), list(data.get("boards") or [])


def cockpit_notes(tag: str) -> str:
    min_firmware, boards = manifest_boards((ROOT / MANIFEST).read_text(encoding="utf-8"))
    if not boards:
        raise ReleaseError(f"{MANIFEST} lists no boards")

    prev_release = previous_tag("hardware-v", tag)
    was: dict[str, str] = {}
    if prev_release:
        try:
            _, prev_boards = manifest_boards(git("show", f"{prev_release}:{MANIFEST}"))
            was = {b["board"]: str(b["version"]) for b in prev_boards}
        except subprocess.CalledProcessError:
            pass  # the previous release predates the manifest (a baseline)

    rows, sections = [], []
    for entry in boards:
        board, path, version = entry["board"], entry["path"], str(entry["version"])
        board_tag = f"pcb/{board}-v{version}"
        if not tag_exists(board_tag):
            raise ReleaseError(f"{board} {version} has no {board_tag} tag — release the board first")
        before = was.get(board)
        if before is None:
            change = "new"
        elif before == version:
            change = "unchanged"
        else:
            change = f"{before} → {version}"
        rows.append(f"| {board} | {version} | {change} |")
        body = (
            f"Unchanged since {prev_release}."
            if before == version
            else board_notes(board, pathlib.PurePosixPath(path), board_tag)
        )
        sections.append(f"## {board} {version}\n\n{body}")

    header = [
        f"**Minimum firmware:** {min_firmware or 'not set'}",
        "",
        "| Board | Version | Change |",
        "|---|---|---|",
        *rows,
    ]
    return "\n".join(header) + "\n\n" + "\n\n".join(sections)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("tag")
    parser.add_argument("--dry-run", action="store_true",
                        help="preview a tag that doesn't exist yet; the range ends at HEAD")
    parser.add_argument("--notes", type=pathlib.Path, help="write the notes here instead of stdout")
    args = parser.parse_args()

    match = TAG_RE.match(args.tag)
    if not match:
        print(f"error: {args.tag} is not a hardware release tag", file=sys.stderr)
        return 1
    version = match["version"]
    if version == "0.0.0":
        print(f"{args.tag} is a baseline tag: no release.", file=sys.stderr)
        return 0

    try:
        if not args.dry_run:
            if not tag_exists(args.tag):
                raise ReleaseError(f"{args.tag} does not exist (use --dry-run to preview)")
            if git("cat-file", "-t", f"refs/tags/{args.tag}").strip() != "tag":
                raise ReleaseError(f"{args.tag} is a lightweight tag — release tags must be annotated (git tag -a)")
        if match["board"]:
            board = match["board"]
            folder = board_folder(board)
            check_revision(board, folder, version)
            title, notes = f"{board} {version}", board_notes(board, folder, args.tag)
        else:
            title, notes = f"Hardware {version}", cockpit_notes(args.tag)
    except ReleaseError as err:
        print(f"error: {err}", file=sys.stderr)
        return 1

    print(title)
    if args.notes:
        args.notes.write_text(notes + "\n", encoding="utf-8")
    else:
        print(notes)
    return 0


if __name__ == "__main__":
    sys.exit(main())
