# gen_a4ec — A4EC Header Generator

Python 3 script that parses the DCS-BIOS A-4E-C control definitions and emits
four files into `Firmware/Libraries/A4EC/`:

| File | Contents |
|---|---|
| `A4EC_CmdIds.h` | `DCSIN_*` compact CAN transport IDs for panel inputs |
| `A4EC_OutputIds.h` | `A_4E_C_*` / `_AM` DCS-BIOS output address/mask constants |
| `A4EC_InputMap.h` | `DcsBiosInputEntry[]` dispatch table for PanelBridge |
| `GENERATOR_GAPS.md` | Every skipped control with the reason it was not generated |

All generated files are committed to the repo. The PlatformIO build depends only
on the committed headers — no Python or JSON at compile time.

## Running

From the repo root:

```bash
python tools/gen_a4ec/gen_a4ec.py
```

For reproducible automation, pass the release timestamp and source label
explicitly:

```bash
python tools/gen_a4ec/gen_a4ec.py \
  --source tools/gen_a4ec/data/A-4E-C.jsonp \
  --source-label "DCS-BIOS v0.0.0 (DCS-BIOS_0.0.0.zip)" \
  --timestamp "2025-01-01T00:00:00Z"
```

After running, commit all changed files in `Firmware/Libraries/A4EC/` **and
`tools/gen_a4ec/id_ledger.json`** (the append-only id ledger — the durable source
of truth for `DCSIN_*` assignment), then reflash all boards. PanelBridge and every
PanelGroup node should run firmware built from the same header revision.

`DCSIN_*` ids are **stable across regenerations**: the ledger keeps each control's
id for its lifetime, appends a new control at `max+1`, and retires (never reuses) a
removed control's id — so a snapshot update never renumbers existing controls, it
only appends. Deleting the ledger re-seeds from scratch (renumbers everything), so
keep it committed.

## Source resolution (priority order)

1. `--source <path>` — explicit override, any platform, `.json` or `.jsonp`
2. Platform auto-detect (used if path exists, skipped otherwise):
   - **Windows:** `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\doc_assets\A-4E-C.jsonp`
   - **macOS:** `<repo>/Firmware/ScratchPad/DCS-BIOS/DCS-BIOS/doc/doc_assets/A-4E-C.jsonp`
3. Committed snapshot: `tools/gen_a4ec/data/A-4E-C.jsonp`

## Updating the committed snapshot

The preferred update path is the manual GitHub Actions workflow:

```text
Refresh A4EC Metadata
```

It downloads the latest published DCS-Skunkworks DCS-BIOS release, updates
`tools/gen_a4ec/data/A-4E-C.jsonp`, regenerates the A4EC headers, runs the
generator tests, builds PlatformIO projects that reference `A4EC`, and opens a
pull request if anything changed.

For local updates, copy the file from your DCS-BIOS installation, then
regenerate:


```bash
# Windows (PowerShell)
cp "$env:USERPROFILE\Saved Games\DCS\Scripts\DCS-BIOS\doc\doc_assets\A-4E-C.jsonp" tools/gen_a4ec/data/

# macOS (from repo root — ScratchPad already has it)
cp Firmware/ScratchPad/DCS-BIOS/DCS-BIOS/doc/doc_assets/A-4E-C.jsonp tools/gen_a4ec/data/

python tools/gen_a4ec/gen_a4ec.py
```

Commit both the updated `data/A-4E-C.jsonp` and all changed files in
`Firmware/Libraries/A4EC/`.

## Fetching from GitHub

The DCS-BIOS A-4E-C JSON is auto-generated during releases at:
`https://github.com/DCS-Skunkworks/dcs-bios` — `Scripts/DCS-BIOS/doc/doc_assets/`

The `Refresh A4EC Metadata` workflow performs the GitHub fetch. The local
`--fetch-github` flag is still reserved for a future direct generator fetch mode.

## Running the tests

```bash
python -m pytest tools/gen_a4ec/tests/test_gen.py
```

No external dependencies beyond Python 3 stdlib and `pytest`.
