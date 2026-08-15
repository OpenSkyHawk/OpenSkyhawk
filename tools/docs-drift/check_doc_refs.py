#!/usr/bin/env python3
"""Validate that published docs still cite the code they claim to cite.

Published pages under ``docs/`` reference firmware source by line number, e.g.
``Inputs/AnalogInput/AnalogInput.h:85``. Those citations rot silently whenever the
cited file gains or loses lines above the target. This script enforces:

  * every ``path.ext:N`` citation resolves to a real, non-blank line (Tier A),
  * in a markdown table row that names an identifier and cites a line, the cited
    line actually contains one of the identifiers named in that row (Tier B).

**Tier B is the one that matters.** A citation can rot into pointing at unrelated
but perfectly valid code: #267 inserted a constructor parameter above an ``#ifdef``
guard, moving it 13 lines down, and the stale ``AnalogInput.h:72`` still landed on
a real line of code. No "does this line exist" check can catch that — only asking
whether the line still contains what the row says it contains.

``docs/api/`` (mkdoxy-generated) and ``docs/_source/`` (source, not a published
page) are excluded. Citations of files outside the repository — STM32duino
framework headers, for example — are counted and skipped, never failed.

Deliberately absent: any assertion about *how many* citations or flags exist. A
count assertion fires on every legitimate addition and teaches people to bump the
number without checking anything.

Run from anywhere:

    python3 tools/docs-drift/check_doc_refs.py [--root PATH]

Exit code 0 on success, 1 on any violation.
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

DOCS_DIR = "docs"
EXCLUDED_PREFIXES = ("docs/api/", "docs/_source/")
SKIP_DIRS = {".git", "node_modules", "site", ".venv", "__pycache__"}

SOURCE_EXTS = ("h", "hpp", "c", "cpp", "ini", "py", "yml", "yaml")

# `Some/Path/File.h:123` — the citation form used throughout the published docs.
REF_RE = re.compile(
    r"`([A-Za-z0-9_./+-]+\.(?:" + "|".join(SOURCE_EXTS) + r")):(\d+)`"
)

# A backticked ALL-CAPS token: build flags, macros, guards. Three chars minimum so
# short pin names and prose initialisms don't dominate.
IDENT_RE = re.compile(r"`([A-Z][A-Z0-9_]{2,})`")

# Lines that carry no meaning to cite — a citation landing here is dead.
DEAD_LINES = {"", "//", "/*", "*/", "{", "}", "*", "#"}


def index_repo(root: pathlib.Path) -> dict[str, list[pathlib.Path]]:
    """Map each source file's basename to every repo path with that basename."""
    index: dict[str, list[pathlib.Path]] = {}
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if any(part in SKIP_DIRS for part in path.parts):
            continue
        if path.suffix.lstrip(".") not in SOURCE_EXTS:
            continue
        index.setdefault(path.name, []).append(path)
    return index


def resolve(ref: str, index: dict[str, list[pathlib.Path]], root: pathlib.Path) -> list[pathlib.Path]:
    """Return repo files whose path ends with ``ref``.

    A citation is written relative to some library root (``Inputs/AnalogInput/
    AnalogInput.h``), so match on path suffix rather than trying to guess the base.
    """
    name = ref.rsplit("/", 1)[-1]
    candidates = index.get(name, [])
    if "/" not in ref:
        return sorted(candidates, key=lambda p: len(p.parts))
    suffix = tuple(ref.split("/"))
    hits = [p for p in candidates if p.parts[-len(suffix):] == suffix]
    return sorted(hits, key=lambda p: len(p.parts))


def published_pages(root: pathlib.Path) -> list[pathlib.Path]:
    """Every published markdown page — excludes generated and source-only trees."""
    docs = root / DOCS_DIR
    if not docs.is_dir():
        return []
    out = []
    for path in sorted(docs.rglob("*.md")):
        rel = path.relative_to(root).as_posix()
        if rel.startswith(EXCLUDED_PREFIXES):
            continue
        out.append(path)
    return out


def check_page(
    page: pathlib.Path,
    root: pathlib.Path,
    index: dict[str, list[pathlib.Path]],
) -> tuple[list[str], int, int]:
    """Check one page. Returns (errors, refs_checked, refs_skipped)."""
    errors: list[str] = []
    checked = skipped = 0
    rel_page = page.relative_to(root).as_posix()

    for lineno, line in enumerate(page.read_text(encoding="utf-8").splitlines(), start=1):
        refs = REF_RE.findall(line)
        if not refs:
            continue

        # Tier B applies to table rows only: a row names an identifier and cites
        # where it lives, so the two must agree. Prose has no such pairing.
        is_row = line.lstrip().startswith("|")
        idents = IDENT_RE.findall(line) if is_row else []

        for ref, lineno_str in refs:
            target_line = int(lineno_str)
            matches = resolve(ref, index, root)
            if not matches:
                # Outside the repo — a framework header. Not ours to verify.
                skipped += 1
                continue
            checked += 1

            # A citation is satisfied if ANY matching file satisfies it; several
            # libraries can share a basename.
            tier_a_ok = False
            tier_b_ok = not idents
            best_hint = ""

            for target in matches:
                lines = target.read_text(encoding="utf-8", errors="replace").splitlines()
                if target_line > len(lines):
                    continue
                text = lines[target_line - 1].strip()
                if text not in DEAD_LINES:
                    tier_a_ok = True
                if idents and any(i in text for i in idents):
                    tier_b_ok = True
                    break
                if idents and not best_hint:
                    for ident in idents:
                        real = [n for n, l in enumerate(lines, 1) if ident in l]
                        if real:
                            best_hint = f"{ident} is at {target.relative_to(root).as_posix()}:{real[0]}"
                            break

            if not tier_a_ok:
                errors.append(
                    f"{rel_page}:{lineno}: `{ref}:{target_line}` does not point at code "
                    f"(line is blank, punctuation, or past end of file)"
                )
            elif not tier_b_ok:
                named = ", ".join(idents)
                hint = f" — {best_hint}" if best_hint else ""
                errors.append(
                    f"{rel_page}:{lineno}: `{ref}:{target_line}` no longer contains {named}"
                    f"{hint}"
                )

    return errors, checked, skipped


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--root",
        type=pathlib.Path,
        default=pathlib.Path(__file__).resolve().parent.parent.parent,
        help="repository root (default: infer from this script's location)",
    )
    args = parser.parse_args(argv)
    root = args.root.resolve()

    index = index_repo(root)
    pages = published_pages(root)

    errors: list[str] = []
    checked = skipped = 0
    for page in pages:
        page_errors, page_checked, page_skipped = check_page(page, root, index)
        errors.extend(page_errors)
        checked += page_checked
        skipped += page_skipped

    if errors:
        print("Doc reference validation FAILED:")
        for e in errors:
            print(f"  - {e}")
        print(
            "\nA citation that resolves to real code can still be wrong — check what the "
            "line now says, don't just confirm it exists."
        )
        return 1

    note = f", {skipped} outside the repo skipped" if skipped else ""
    print(
        f"Doc reference validation OK — {checked} citation(s) across "
        f"{len(pages)} published page(s){note}."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
