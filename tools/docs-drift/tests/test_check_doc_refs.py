"""
Tests for check_doc_refs.py — run from repo root:
  python -m pytest tools/docs-drift/tests/test_check_doc_refs.py

Every test builds a throwaway repo tree, so each assertion is shown to *fail on
broken input* rather than merely passing on the real one. The two tier tests are
the point: a check nobody has watched fail is decorative.
"""

import sys
from pathlib import Path

import pytest

TESTS_DIR = Path(__file__).resolve().parent
SCRIPT_DIR = TESTS_DIR.parent
sys.path.insert(0, str(SCRIPT_DIR))

import check_doc_refs as C


# ── Helpers ───────────────────────────────────────────────────────────────────

HEADER = "| Flag | Library | First read at |\n|---|---|---|\n"


def build(tmp_path: Path, source: str, page: str, page_path: str = "docs/firmware/flags.md") -> Path:
    """Write a minimal repo tree: one source file, one published page."""
    src = tmp_path / "Firmware" / "Libraries" / "Widget" / "Widget.h"
    src.parent.mkdir(parents=True, exist_ok=True)
    src.write_text(source, encoding="utf-8")

    doc = tmp_path / page_path
    doc.parent.mkdir(parents=True, exist_ok=True)
    doc.write_text(page, encoding="utf-8")
    return tmp_path


def run(root: Path) -> int:
    return C.main(["--root", str(root)])


# ── Tier A — the citation resolves ────────────────────────────────────────────

def test_tier_a_passes_on_a_real_line(tmp_path, capsys):
    root = build(
        tmp_path,
        source="int a;\nint b;\n#ifdef WIDGET_TEST\n",
        page="See `Widget.h:3` for the seam.\n",
    )
    assert run(root) == 0
    assert "OK" in capsys.readouterr().out


def test_tier_a_fails_on_a_blank_line(tmp_path, capsys):
    root = build(
        tmp_path,
        source="int a;\n\nint c;\n",
        page="See `Widget.h:2` for the seam.\n",
    )
    assert run(root) == 1
    assert "does not point at code" in capsys.readouterr().out


def test_tier_a_fails_on_a_bare_comment_marker(tmp_path, capsys):
    """The STM32Board.cpp:137 case — an insertion pushed the target onto a bare //."""
    root = build(
        tmp_path,
        source="int a;\n//\nint c;\n",
        page="Defined in `Widget.h:2`, which every node links.\n",
    )
    assert run(root) == 1
    assert "does not point at code" in capsys.readouterr().out


def test_tier_a_fails_past_end_of_file(tmp_path, capsys):
    root = build(tmp_path, source="int a;\n", page="See `Widget.h:99`.\n")
    assert run(root) == 1
    assert "does not point at code" in capsys.readouterr().out


# ── Tier B — the citation still means what it claims ──────────────────────────

def test_tier_b_fails_when_the_guard_moved_but_the_line_is_still_code(tmp_path, capsys):
    """The AnalogInput.h:72 case, and the whole reason this script exists.

    The citation resolves to a real, non-blank line — a constructor parameter —
    so Tier A is satisfied. Only Tier B can catch it.
    """
    root = build(
        tmp_path,
        source="int a;\nuint16_t pollMs = 8,\n#ifdef WIDGET_TEST\n",
        page=HEADER + "| `WIDGET_TEST` | Widget | `Widget.h:2` |\n",
    )
    assert run(root) == 1
    out = capsys.readouterr().out
    assert "no longer contains WIDGET_TEST" in out
    # The fix must be copy-paste, not a hunt.
    assert "Widget.h:3" in out


def test_tier_b_passes_when_the_citation_is_correct(tmp_path):
    root = build(
        tmp_path,
        source="int a;\nuint16_t pollMs = 8,\n#ifdef WIDGET_TEST\n",
        page=HEADER + "| `WIDGET_TEST` | Widget | `Widget.h:3` |\n",
    )
    assert run(root) == 0


def test_tier_b_accepts_any_identifier_named_in_the_row(tmp_path):
    """Rows often name a flag *and* its default (`SHIFTBUS_SCK` … `PB3`).

    Matching only the first would fail the row spuriously, so any identifier in
    the row satisfies it.
    """
    root = build(
        tmp_path,
        source="int a;\n#ifndef SHIFTBUS_SCK\n#define SHIFTBUS_SCK PB3\n",
        page="| `SHIFTBUS_SCK` | `PB3` | `Widget.h:2` |\n",
    )
    assert run(root) == 0


def test_tier_b_does_not_apply_to_prose(tmp_path):
    """Prose names identifiers without claiming the citation defines them."""
    root = build(
        tmp_path,
        source="int a;\nint b;\n#ifdef WIDGET_TEST\n",
        page="`WIDGET_TEST` is documented near `Widget.h:2` in passing.\n",
    )
    assert run(root) == 0


# ── Scope ─────────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("excluded", ["docs/api/generated.md", "docs/_source/notes.md"])
def test_generated_and_source_trees_are_ignored(tmp_path, excluded):
    root = build(
        tmp_path,
        source="int a;\n\nint c;\n",
        page="Broken ref `Widget.h:2`.\n",
        page_path=excluded,
    )
    assert run(root) == 0


def test_files_outside_the_repo_are_skipped_not_failed(tmp_path, capsys):
    """Framework headers (STM32duino) aren't ours to verify."""
    root = build(
        tmp_path,
        source="int a;\n",
        page="Set in `stm32f1xx_hal_conf_default.h:321`.\n",
    )
    assert run(root) == 0
    assert "outside the repo skipped" in capsys.readouterr().out


def test_a_shared_basename_passes_if_any_match_satisfies(tmp_path):
    """Two libraries can both ship a Widget.h; the citation names neither fully."""
    root = build(
        tmp_path,
        source="int a;\n\nint c;\n",
        page="See `Widget.h:2`.\n",
    )
    other = tmp_path / "Firmware" / "Libraries" / "Other" / "Widget.h"
    other.parent.mkdir(parents=True, exist_ok=True)
    other.write_text("int a;\nint real_code;\nint c;\n", encoding="utf-8")
    assert run(root) == 0


def test_a_path_qualified_citation_is_not_satisfied_by_a_different_library(tmp_path):
    """`Other/Widget.h:2` must resolve against Other/, not any Widget.h."""
    root = build(
        tmp_path,
        source="int a;\nint real_code;\n",
        page="See `Other/Widget.h:2`.\n",
    )
    other = tmp_path / "Firmware" / "Libraries" / "Other" / "Widget.h"
    other.parent.mkdir(parents=True, exist_ok=True)
    other.write_text("int a;\n\n", encoding="utf-8")
    assert run(root) == 1


def test_no_citations_is_not_an_error(tmp_path):
    root = build(tmp_path, source="int a;\n", page="Prose with no citations.\n")
    assert run(root) == 0
