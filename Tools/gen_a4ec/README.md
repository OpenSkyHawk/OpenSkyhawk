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
python Tools/gen_a4ec/gen_a4ec.py
```

After running, commit all changed files in `Firmware/Libraries/A4EC/` and reflash
all boards. PanelBridge and every PanelGroup node must run firmware built from
the same header revision — `DCSIN_*` IDs are not backward-compatible across
regenerations.

## Source resolution (priority order)

1. `--source <path>` — explicit override, any platform, `.json` or `.jsonp`
2. Platform auto-detect (used if path exists, skipped otherwise):
   - **Windows:** `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\doc_assets\A-4E-C.jsonp`
   - **macOS:** `<repo>/Firmware/ScratchPad/DCS-BIOS/DCS-BIOS/doc/doc_assets/A-4E-C.jsonp`
3. Committed snapshot: `Tools/gen_a4ec/data/A-4E-C.jsonp`

## Updating the committed snapshot

Copy the file from your DCS-BIOS installation, then regenerate:

```bash
# Windows (PowerShell)
cp "$env:USERPROFILE\Saved Games\DCS\Scripts\DCS-BIOS\doc\doc_assets\A-4E-C.jsonp" Tools/gen_a4ec/data/

# macOS (from repo root — ScratchPad already has it)
cp Firmware/ScratchPad/DCS-BIOS/DCS-BIOS/doc/doc_assets/A-4E-C.jsonp Tools/gen_a4ec/data/

python Tools/gen_a4ec/gen_a4ec.py
```

Commit both the updated `data/A-4E-C.jsonp` and all changed files in
`Firmware/Libraries/A4EC/`.

## Fetching from GitHub (not yet implemented)

The DCS-BIOS A-4E-C JSON is auto-generated during releases at:
`https://github.com/DCS-Skunkworks/dcs-bios` — `Scripts/DCS-BIOS/doc/doc_assets/`

A future `--fetch-github` flag will download the latest release asset and
update the committed snapshot automatically. For now, use the manual copy
procedure above.

## Running the tests

```bash
python -m pytest Tools/gen_a4ec/tests/test_gen.py
```

No external dependencies beyond Python 3 stdlib and `pytest`.
