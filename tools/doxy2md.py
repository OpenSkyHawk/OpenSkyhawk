#!/usr/bin/env python3
"""Convert Doxygen XML output to Markdown API docs for MkDocs.

Usage:
    python3 tools/doxy2md.py --xml .doxygen/xml --out docs/api

One .md file is generated per compound (namespace, class, struct, file).
Anonymous namespaces, dir/page compounds, and private members are skipped.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from xml.etree import ElementTree as ET


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _safe_filename(name: str) -> str:
    """Convert a compound name to a filesystem-safe filename stem."""
    safe = name.replace("::", "_").replace(".", "_").replace("/", "_")
    safe = re.sub(r"[^A-Za-z0-9_\-]", "_", safe)
    return safe


def _flatten(elem: ET.Element | None) -> str:
    """Recursively flatten a Doxygen XML description element to plain text."""
    if elem is None:
        return ""
    parts: list[str] = []
    if elem.text:
        parts.append(elem.text.strip())
    for child in elem:
        tag = child.tag
        if tag == "ref":
            parts.append(child.text or "")
        elif tag == "para":
            t = _flatten(child)
            if t:
                parts.append(t)
        elif tag == "programlisting":
            code = _code_text(child)
            if code:
                parts.append(f"\n```cpp\n{code}\n```\n")
        elif tag == "itemizedlist":
            for li in child.findall("listitem"):
                item = _flatten(li)
                if item:
                    parts.append(f"- {item}")
        elif tag == "orderedlist":
            for i, li in enumerate(child.findall("listitem"), 1):
                item = _flatten(li)
                if item:
                    parts.append(f"{i}. {item}")
        elif tag == "sp":
            parts.append(" ")
        elif tag in ("emphasis", "bold", "computeroutput"):
            inner = _flatten(child)
            if inner:
                parts.append(f"`{inner}`" if tag == "computeroutput" else inner)
        else:
            parts.append(_flatten(child))
        if child.tail:
            parts.append(child.tail.strip())
    return " ".join(p for p in parts if p)


def _code_text(elem: ET.Element) -> str:
    """Extract source text from a <programlisting> element."""
    lines: list[str] = []
    for codeline in elem.findall("codeline"):
        line_parts: list[str] = []
        for hl in codeline.findall("highlight"):
            if hl.text:
                line_parts.append(hl.text)
            for sp in hl:
                if sp.text:
                    line_parts.append(sp.text)
                if sp.tail:
                    line_parts.append(sp.tail)
        lines.append("".join(line_parts))
    return "\n".join(lines)


def _brief(elem: ET.Element) -> str:
    return _flatten(elem.find("briefdescription")).strip()


def _detail(elem: ET.Element) -> str:
    return _flatten(elem.find("detaileddescription")).strip()


def _params_signature(memberdef: ET.Element) -> str:
    params: list[str] = []
    for param in memberdef.findall("param"):
        type_text = _flatten(param.find("type")).strip() if param.find("type") is not None else ""
        name_elem = param.find("declname")
        name = name_elem.text.strip() if name_elem is not None and name_elem.text else ""
        defval_elem = param.find("defval")
        defval = f" = {_flatten(defval_elem).strip()}" if defval_elem is not None else ""
        parts = " ".join(p for p in [type_text, name] if p)
        params.append(f"{parts}{defval}")
    return ", ".join(params)


def _render_param_docs(memberdef: ET.Element) -> list[str]:
    """Extract @param/@return/@note blocks as Markdown bullet points."""
    lines: list[str] = []
    detail_elem = memberdef.find("detaileddescription")
    if detail_elem is None:
        return lines
    for para in detail_elem.findall("para"):
        for plist in para.findall("parameterlist"):
            kind = plist.get("kind", "param")
            for pitem in plist.findall("parameteritem"):
                names = [n.text for n in pitem.findall(".//parametername") if n.text]
                pdesc = _flatten(pitem.find("parameterdescription")).strip()
                if names and pdesc:
                    label = "return" if kind == "retval" else f"`{'`, `'.join(names)}`"
                    lines.append(f"- **{label}** — {pdesc}")
        for ss in para.findall("simplesect"):
            kind = ss.get("kind", "")
            label_map = {
                "return": "Returns",
                "note": "Note",
                "warning": "Warning",
                "see": "See also",
                "pre": "Precondition",
                "post": "Postcondition",
            }
            label = label_map.get(kind, kind.title())
            content = _flatten(ss).strip()
            if content:
                lines.append(f"**{label}:** {content}")
    return lines


# ---------------------------------------------------------------------------
# Compound renderer
# ---------------------------------------------------------------------------

def render_compound(root: ET.Element, out_dir: Path) -> Path | None:
    """Render one Doxygen XML compound root to a Markdown file.

    Returns the output path on success, or None if the compound is skipped.
    """
    cdef = root.find("compounddef")
    if cdef is None:
        return None

    kind = cdef.get("kind", "")
    if kind in ("dir", "page", "example", "group"):
        return None

    name_elem = cdef.find("compoundname")
    if name_elem is None or not name_elem.text:
        return None
    name = name_elem.text.strip()

    if "@" in name:  # anonymous namespace
        return None

    out_path = out_dir / (_safe_filename(name) + ".md")
    buf: list[str] = []

    def w(*args: str) -> None:
        buf.append(" ".join(args))

    w(f"# {name}")
    w("")

    brief = _brief(cdef)
    if brief:
        w(f"> {brief}")
        w("")

    detail = _detail(cdef)
    if detail and detail != brief:
        w(detail)
        w("")

    # ── Enumerations ──────────────────────────────────────────────────────
    enums = [
        m for sd in cdef.findall("sectiondef")
        for m in sd.findall("memberdef")
        if m.get("kind") == "enum"
    ]
    if enums:
        w("## Enumerations")
        w("")
        for mdef in enums:
            ename = mdef.findtext("name", "").strip()
            ebrief = _brief(mdef)
            edetail = _detail(mdef)
            w(f"### `{ename}`")
            if ebrief:
                w(ebrief)
            if edetail and edetail != ebrief:
                w("")
                w(edetail)
            w("")
            for val in mdef.findall("enumvalue"):
                vname = val.findtext("name", "").strip()
                vbrief = _brief(val)
                init = val.findtext("initializer", "").strip()
                suffix = f" = `{init}`" if init else ""
                desc = f" — {vbrief}" if vbrief else ""
                w(f"- `{vname}`{suffix}{desc}")
            w("")

    # ── Functions (public/protected only) ─────────────────────────────────
    functions = [
        m for sd in cdef.findall("sectiondef")
        for m in sd.findall("memberdef")
        if m.get("kind") == "function" and m.get("prot") not in ("private",)
    ]
    if functions:
        w("## Functions")
        w("")
        for mdef in functions:
            ret_type = _flatten(mdef.find("type")).strip() if mdef.find("type") is not None else ""
            fname = mdef.findtext("name", "").strip()
            params = _params_signature(mdef)
            fbrief = _brief(mdef)
            fdetail = _detail(mdef)
            static_kw = "static " if mdef.get("static") == "yes" else ""
            const_kw = " const" if mdef.get("const") == "yes" else ""
            w(f"### `{static_kw}{ret_type} {fname}({params}){const_kw}`")
            if fbrief:
                w(fbrief)
            if fdetail and fdetail != fbrief:
                w("")
                w(fdetail)
            pdocs = _render_param_docs(mdef)
            if pdocs:
                w("")
                for line in pdocs:
                    w(line)
            w("")

    # ── Constants / variables / defines ───────────────────────────────────
    constants = [
        m for sd in cdef.findall("sectiondef")
        for m in sd.findall("memberdef")
        if m.get("kind") in ("variable", "define") and m.get("prot") not in ("private",)
    ]
    if constants:
        w("## Constants")
        w("")
        for mdef in constants:
            cname = mdef.findtext("name", "").strip()
            cbrief = _brief(mdef)
            init = _flatten(mdef.find("initializer")).strip() if mdef.find("initializer") is not None else ""
            suffix = f" = `{init}`" if init else ""
            desc = f" — {cbrief}" if cbrief else ""
            w(f"- `{cname}`{suffix}{desc}")
        w("")

    out_path.write_text("\n".join(buf), encoding="utf-8")
    return out_path


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert Doxygen XML output to Markdown API docs."
    )
    parser.add_argument("--xml", required=True, metavar="DIR",
                        help="Path to Doxygen XML output directory (contains index.xml)")
    parser.add_argument("--out", required=True, metavar="DIR",
                        help="Output directory for generated Markdown files")
    args = parser.parse_args()

    xml_dir = Path(args.xml)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    index_path = xml_dir / "index.xml"
    if not index_path.exists():
        print(f"error: {index_path} not found — run doxygen first.", file=sys.stderr)
        sys.exit(1)

    index = ET.parse(index_path)
    generated = 0
    skipped = 0

    for ref in index.findall("compound"):
        refid = ref.get("refid")
        if not refid:
            continue
        xml_file = xml_dir / f"{refid}.xml"
        if not xml_file.exists():
            continue
        try:
            tree = ET.parse(xml_file)
        except ET.ParseError as exc:
            print(f"warning: skipping {xml_file.name}: {exc}", file=sys.stderr)
            skipped += 1
            continue
        result = render_compound(tree.getroot(), out_dir)
        if result:
            print(f"  {result.name}")
            generated += 1
        else:
            skipped += 1

    print(f"Generated {generated} files, skipped {skipped} compounds.")


if __name__ == "__main__":
    main()
