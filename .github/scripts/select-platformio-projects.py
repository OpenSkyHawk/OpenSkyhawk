#!/usr/bin/env python3
"""Select PlatformIO firmware projects affected by a change set.

PR builds use the affected set. Scheduled and manual builds use the full set.
The dependency knowledge is intentionally explicit and lives in
.github/firmware-platformio-impact.json so new panel sketches can be added
without reverse-engineering PlatformIO's library resolver in CI.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


DOC_SUFFIXES = {
    ".gif",
    ".jpeg",
    ".jpg",
    ".md",
    ".pdf",
    ".png",
    ".rst",
    ".svg",
    ".txt",
    ".webp",
}


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def posix(path: Path | str) -> str:
    return Path(path).as_posix()


def load_impact_map(root: Path) -> dict:
    map_path = root / ".github" / "firmware-platformio-impact.json"
    with map_path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def discover_projects(root: Path) -> list[str]:
    projects: list[str] = []
    firmware_root = root / "Firmware"
    for ini in firmware_root.rglob("platformio.ini"):
        rel = ini.relative_to(root)
        if "ScratchPad" in rel.parts:
            continue
        projects.append(posix(rel.parent))
    return sorted(projects)


def changed_files_from_git(base: str | None, head: str | None) -> list[str]:
    if not base:
        raise SystemExit("--base is required for affected builds unless --changed-file is used")

    revspec = f"{base}...{head or 'HEAD'}"
    result = subprocess.run(
        ["git", "diff", "--name-only", revspec],
        check=True,
        text=True,
        capture_output=True,
    )
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def path_matches(path: str, watched: str) -> bool:
    watched = watched.rstrip("/")
    return path == watched or path.startswith(f"{watched}/")


def nearest_project_for_path(root: Path, projects: set[str], path: str) -> str | None:
    candidate = root / path
    if candidate.name == "platformio.ini":
        candidate = candidate.parent
    elif not candidate.is_dir():
        candidate = candidate.parent

    while candidate != root and root in candidate.parents:
        rel = posix(candidate.relative_to(root))
        if rel in projects:
            return rel
        candidate = candidate.parent
    return None


def is_doc_like(path: str) -> bool:
    return Path(path).suffix.lower() in DOC_SUFFIXES


def select_projects(
    *,
    mode: str,
    changed_files: list[str],
    all_projects: list[str],
    impact_map: dict,
    root: Path,
) -> tuple[list[str], str]:
    if mode == "full":
        return all_projects, "full"

    project_set = set(all_projects)
    selected: set[str] = set()

    full_build_paths = impact_map.get("full_build_paths", [])
    library_dependents = impact_map.get("library_dependents", {})

    for changed in changed_files:
        changed = changed.strip("/")
        if not changed:
            continue

        if any(path_matches(changed, watched) for watched in full_build_paths):
            return all_projects, f"full: {changed} affects CI selection"

        if changed.startswith("Firmware/ScratchPad/"):
            continue

        if changed.startswith("Firmware/") and is_doc_like(changed):
            continue

        direct_project = nearest_project_for_path(root, project_set, changed)
        if direct_project:
            selected.add(direct_project)
            continue

        parts = Path(changed).parts
        if len(parts) >= 3 and parts[0] == "Firmware" and parts[1] == "Libraries":
            library = parts[2]
            dependents = library_dependents.get(library)
            if dependents is None:
                return all_projects, f"full: unknown library {library}"

            for project in dependents:
                if project in project_set:
                    selected.add(project)
                else:
                    print(
                        f"warning: impact map references missing project {project}",
                        file=sys.stderr,
                    )
            continue

        if changed.startswith("Firmware/"):
            return all_projects, f"full: unknown firmware path {changed}"

    return sorted(selected), "affected"


def write_project_list(path: Path, projects: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(f"{project}\n" for project in projects), encoding="utf-8")


def write_github_output(path: Path, projects: list[str], scope: str) -> None:
    with path.open("a", encoding="utf-8") as handle:
        handle.write(f"project_count={len(projects)}\n")
        handle.write(f"build_scope={scope}\n")
        handle.write("projects<<EOF\n")
        for project in projects:
            handle.write(f"{project}\n")
        handle.write("EOF\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=["affected", "full"], required=True)
    parser.add_argument("--base")
    parser.add_argument("--head")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--github-output", type=Path)
    parser.add_argument(
        "--changed-file",
        action="append",
        default=[],
        help="Inject a changed file for local tests; may be repeated.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = repo_root()
    impact_map = load_impact_map(root)
    all_projects = discover_projects(root)

    changed_files = (
        args.changed_file
        if args.changed_file
        else ([] if args.mode == "full" else changed_files_from_git(args.base, args.head))
    )

    projects, scope = select_projects(
        mode=args.mode,
        changed_files=changed_files,
        all_projects=all_projects,
        impact_map=impact_map,
        root=root,
    )

    print(f"Firmware build scope: {scope}")
    if changed_files:
        print("Changed files:")
        for changed in changed_files:
            print(f"  {changed}")

    if projects:
        print("Selected PlatformIO projects:")
        for project in projects:
            print(f"  {project}")
    else:
        print("No PlatformIO projects selected.")

    write_project_list(args.output, projects)
    if args.github_output:
        write_github_output(args.github_output, projects, scope)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
