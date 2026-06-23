"""
Tests for gen_a4ec.py — run from repo root:
  python -m pytest tools/gen_a4ec/tests/test_gen.py
"""

import json
import sys
import tempfile
from pathlib import Path

import pytest

# Make the generator importable regardless of working directory
TESTS_DIR = Path(__file__).resolve().parent
SCRIPT_DIR = TESTS_DIR.parent
sys.path.insert(0, str(SCRIPT_DIR))

import gen_a4ec as G

FIXTURE = TESTS_DIR / "fixture.jsonp"


# ── Helpers ───────────────────────────────────────────────────────────────────

def load_fixture() -> dict:
    return G.flatten_controls(G.load_data(FIXTURE))


def run_against_fixture(ledger_path=None) -> tuple:
    controls = load_fixture()
    if ledger_path is None:
        # Isolated, non-existent path → fresh sequential assignment; never touches the real ledger.
        ledger_path = Path(tempfile.mkdtemp()) / "ledger.json"
    input_entries, input_gaps = G.build_input_entries(controls, ledger_path)
    output_entries, output_gaps = G.build_output_entries(controls)
    return controls, input_entries, input_gaps, output_entries, output_gaps


# ── JSONP parsing ─────────────────────────────────────────────────────────────

def test_jsonp_stripping():
    data = G.load_data(FIXTURE)
    assert isinstance(data, dict)
    assert "TEST" in data


def test_plain_json_also_parses(tmp_path):
    plain = tmp_path / "plain.json"
    plain.write_text('{"CAT": {"X": {"inputs": [], "outputs": []}}}')
    data = G.load_data(plain)
    assert "CAT" in data


# ── Flatten ───────────────────────────────────────────────────────────────────

def test_flatten_removes_category_level():
    controls = load_fixture()
    # top-level keys should be control names, not category names
    assert "ALPHA_SWITCH" in controls
    assert "TEST" not in controls


# classify_input + InputType were removed in #147 — the map is now controlId → name only, and the
# dispatch form comes from the PanelGroup input class via the CAN frame it emits on. The collapse
# is covered by the emitted-content + real-data regression tests further down.


# ── build_input_entries ───────────────────────────────────────────────────────

def test_id_assignment_starts_at_base():
    _, entries, _, _, _ = run_against_fixture()
    assert entries[0]["cmdId"] == G.DCSIN_BASE


def test_id_assignment_sequential():
    _, entries, _, _, _ = run_against_fixture()
    for i, e in enumerate(entries):
        assert e["cmdId"] == G.DCSIN_BASE + i


def test_entries_alphabetical_order():
    _, entries, _, _, _ = run_against_fixture()
    names = [e["name"] for e in entries]
    assert names == sorted(names)


def test_mapped_controls_are_correct():
    _, entries, _, _, _ = run_against_fixture()
    names = {e["name"] for e in entries}
    # These six should be mapped
    assert "ALPHA_SWITCH"  in names
    assert "BETA_MULTIPOS" in names
    assert "DELTA_ENCODER" in names
    assert "EPSILON_ACCEL" in names
    assert "GAMMA_ACTION"  in names
    assert "IOTA_VARSTEP"  in names   # ANALOG type
    assert "ZETA_COVER"   in names   # SWITCH type — guard covers are plain 2-pos switches


def test_no_gaps():
    _, _, input_gaps, _, output_gaps = run_against_fixture()
    assert input_gaps == []
    assert output_gaps == []


def test_map_size_constant_matches_entries():
    _, entries, _, _, _ = run_against_fixture()
    ts = "2025-01-01T00:00:00Z"
    header = G.emit_inputmap(entries, "fixture.jsonp", ts)
    assert f"sizeof(A4EC_INPUT_MAP) / sizeof(A4EC_INPUT_MAP[0])" in header


# ── build_output_entries ──────────────────────────────────────────────────────

def test_output_address_constant_name():
    _, _, _, out_entries, _ = run_against_fixture()
    addr_ids = {e["addr_id"] for e in out_entries}
    assert "A_4E_C_ALPHA_SWITCH" in addr_ids


def test_output_mask_id_from_json():
    _, _, _, out_entries, _ = run_against_fixture()
    by_addr = {e["addr_id"]: e for e in out_entries}
    # ALPHA_SWITCH has address_mask_identifier in fixture
    assert by_addr["A_4E_C_ALPHA_SWITCH"]["mask_id"] == "A_4E_C_ALPHA_SWITCH_AM"


def test_output_mask_id_fallback_suffix():
    _, _, _, out_entries, _ = run_against_fixture()
    by_addr = {e["addr_id"]: e for e in out_entries}
    # BETA_MULTIPOS has no address_mask_identifier — should get _AM appended
    assert by_addr["A_4E_C_BETA_MULTIPOS"]["mask_id"] == "A_4E_C_BETA_MULTIPOS_AM"


def test_output_address_hex_value():
    _, _, _, out_entries, _ = run_against_fixture()
    by_addr = {e["addr_id"]: e for e in out_entries}
    assert by_addr["A_4E_C_ALPHA_SWITCH"]["address"] == 4096


def test_output_alphabetical_by_identifier():
    _, _, _, out_entries, _ = run_against_fixture()
    ids = [e["addr_id"] for e in out_entries]
    assert ids == sorted(ids)


def test_output_multi_entry_per_control(tmp_path):
    multi = tmp_path / "multi.json"
    multi.write_text(json.dumps({
        "CAT": {
            "CTRL": {
                "description": "multi-output control",
                "inputs": [],
                "outputs": [
                    {"address": 100, "mask": 1,
                     "address_mask_shift_identifier": "A_4E_C_CTRL_A",
                     "address_mask_identifier":       "A_4E_C_CTRL_A_AM"},
                    {"address": 102, "mask": 2,
                     "address_mask_shift_identifier": "A_4E_C_CTRL_B",
                     "address_mask_identifier":       "A_4E_C_CTRL_B_AM"},
                ],
            }
        }
    }))
    data = G.load_data(multi)
    controls = G.flatten_controls(data)
    out_entries, gaps = G.build_output_entries(controls)
    addr_ids = {e["addr_id"] for e in out_entries}
    assert "A_4E_C_CTRL_A" in addr_ids
    assert "A_4E_C_CTRL_B" in addr_ids
    assert not gaps


# ── Emitted file content ──────────────────────────────────────────────────────

def test_cmdids_pragma_once():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_cmdids(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "#pragma once" in content


def test_cmdids_defines_present():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_cmdids(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "#define DCSIN_ALPHA_SWITCH" in content
    assert "0x8001" in content


def test_cmdids_skipped_not_present():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_cmdids(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "DCSIN_IOTA_VARSTEP" in content
    assert "DCSIN_ZETA_COVER"   in content


def test_outputids_pragma_once():
    _, _, _, out_entries, _ = run_against_fixture()
    content = G.emit_outputids(out_entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "#pragma once" in content


def test_outputids_hex_addresses():
    _, _, _, out_entries, _ = run_against_fixture()
    content = G.emit_outputids(out_entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "0x1000" in content   # 4096 == 0x1000


def test_inputmap_includes_cmdids():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert '#include <A4EC_CmdIds.h>' in content


def test_inputmap_collapsed_to_name_only():
    """#147: rows are { DCSIN_<name>, "<name>" } — no InputType, no per-control arg columns."""
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert 'InputType' not in content                       # no dispatch type codes
    assert 'arg0' not in content                            # no arg columns
    assert 'uint16_t    cmdId;' in content and 'const char* name;' in content
    assert '{ DCSIN_ALPHA_SWITCH, "ALPHA_SWITCH" }' in content
    # the variable_step knob that used to be force-classified ANALOG is now just a name
    assert '{ DCSIN_IOTA_VARSTEP, "IOTA_VARSTEP" }' in content


def test_real_data_map_is_name_only(tmp_path):
    """Regression vs the committed snapshot: a known variable_step knob maps to name only —
    not a forced dispatch type (the #147 break)."""
    real = G.flatten_controls(G.load_data(G.COMMITTED_SNAPSHOT))
    entries, gaps = G.build_input_entries(real, tmp_path / "ledger.json")
    assert gaps == []                                       # every input control maps
    by_name = {e["name"]: e for e in entries}
    assert "ASN41_MAGVAR_KNB" in by_name
    assert set(by_name["ASN41_MAGVAR_KNB"].keys()) == {"name", "cmdId"}   # no type/args
    content = G.emit_inputmap(entries, "A-4E-C.jsonp", "2025-01-01T00:00:00Z")
    assert '{ DCSIN_ASN41_MAGVAR_KNB, "ASN41_MAGVAR_KNB" }' in content
    assert 'InputType' not in content


def test_gaps_file_always_emitted():
    _, _, input_gaps, _, output_gaps = run_against_fixture()
    content = G.emit_gaps(input_gaps + output_gaps, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "# A4EC Generator Gaps" in content


def test_gaps_file_lists_skipped_controls():
    _, _, input_gaps, _, output_gaps = run_against_fixture()
    content = G.emit_gaps(input_gaps + output_gaps, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert "IOTA_VARSTEP" not in content
    assert "ZETA_COVER"   not in content


# ── Integration: full run against fixture ─────────────────────────────────────

def test_full_run_writes_all_files(tmp_path, monkeypatch):
    monkeypatch.setattr(G, "OUTPUT_DIR", tmp_path)

    controls = load_fixture()
    ts = "2025-01-01T00:00:00Z"
    input_entries,  input_gaps  = G.build_input_entries(controls, tmp_path / "ledger.json")
    output_entries, output_gaps = G.build_output_entries(controls)
    all_gaps = input_gaps + output_gaps

    source_name = "fixture.jsonp"
    writes = {
        "A4EC_CmdIds.h":     G.emit_cmdids(input_entries,   source_name, ts),
        "A4EC_OutputIds.h":  G.emit_outputids(output_entries, source_name, ts),
        "A4EC_InputMap.h":   G.emit_inputmap(input_entries,   source_name, ts),
        "GENERATOR_GAPS.md": G.emit_gaps(all_gaps, source_name, ts),
    }
    for filename, content in writes.items():
        (tmp_path / filename).write_text(content, encoding="utf-8")

    for filename in writes:
        assert (tmp_path / filename).exists(), f"{filename} was not written"


# ── append-only id ledger ─────────────────────────────────────────────────────

def _switch_control() -> dict:
    return {
        "inputs": [{"interface": "action", "argument": "TOGGLE"},
                   {"interface": "fixed_step"},
                   {"interface": "set_state", "max_value": 1}],
        "outputs": [],
    }


def test_ledger_fresh_run_sequential_from_base(tmp_path):
    """No ledger → controls numbered alphabetically from DCSIN_BASE (matches historical output)."""
    ledger = tmp_path / "ledger.json"
    entries, _ = G.build_input_entries(load_fixture(), ledger)
    assert entries[0]["cmdId"] == G.DCSIN_BASE
    assert [e["cmdId"] for e in entries] == [G.DCSIN_BASE + i for i in range(len(entries))]
    assert ledger.exists()


def test_ledger_regen_is_noop(tmp_path):
    """Second run reuses every id — no renumbering."""
    ledger = tmp_path / "ledger.json"
    controls = load_fixture()
    first, _  = G.build_input_entries(controls, ledger)
    second, _ = G.build_input_entries(controls, ledger)
    assert {e["name"]: e["cmdId"] for e in first} == {e["name"]: e["cmdId"] for e in second}


def test_ledger_new_control_appends_at_end(tmp_path):
    """An alphabetically-first new control gets max+1; every existing id is unchanged."""
    ledger = tmp_path / "ledger.json"
    controls = load_fixture()
    before = {e["name"]: e["cmdId"] for e in G.build_input_entries(controls, ledger)[0]}
    max_id = max(before.values())

    controls["AAA_NEW_SWITCH"] = _switch_control()   # sorts first → would shift ids under the old scheme
    after = {e["name"]: e["cmdId"] for e in G.build_input_entries(controls, ledger)[0]}

    for name, cid in before.items():
        assert after[name] == cid, f"{name} renumbered {cid:#x} -> {after[name]:#x}"
    assert after["AAA_NEW_SWITCH"] == max_id + 1


def test_ledger_removed_control_id_retired(tmp_path):
    """A removed control keeps its id reserved; a later new control does not reuse it."""
    ledger = tmp_path / "ledger.json"
    controls = load_fixture()
    before = {e["name"]: e["cmdId"] for e in G.build_input_entries(controls, ledger)[0]}
    removed = max(before, key=lambda n: before[n])
    removed_id = before[removed]

    del controls[removed]
    controls["ZZZ_NEW_SWITCH"] = _switch_control()
    after = {e["name"]: e["cmdId"] for e in G.build_input_entries(controls, ledger)[0]}

    assert after["ZZZ_NEW_SWITCH"] != removed_id          # retired id not reused
    assert after["ZZZ_NEW_SWITCH"] == removed_id + 1      # appended past the retired max
    saved = json.loads(ledger.read_text())
    assert saved.get(removed) == removed_id               # ledger retains the retired id


def test_inputmap_emitted_in_cmdid_order(tmp_path):
    """InputMap rows ascend by cmdId (binary-search invariant) even when an append breaks name order."""
    import re
    ledger = tmp_path / "ledger.json"
    controls = load_fixture()
    G.build_input_entries(controls, ledger)
    controls["AAA_NEW_SWITCH"] = _switch_control()       # name-first but id-last (appended)
    entries, _ = G.build_input_entries(controls, ledger)

    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    rows = re.findall(r"\{ DCSIN_(\w+),", content)
    by_name = {e["name"]: e["cmdId"] for e in entries}
    ordered_ids = [by_name[n] for n in rows]
    assert ordered_ids == sorted(ordered_ids)             # rows ascend by cmdId
    assert rows[0] != "AAA_NEW_SWITCH"                    # not first despite alphabetical name
