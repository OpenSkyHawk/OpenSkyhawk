"""
Tests for gen_a4ec.py — run from repo root:
  python -m pytest Tools/gen_a4ec/tests/test_gen.py
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


def run_against_fixture() -> tuple:
    controls = load_fixture()
    input_entries, input_gaps = G.build_input_entries(controls)
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


# ── classify_input ────────────────────────────────────────────────────────────

def test_switch_mapping():
    inputs = [
        {"interface": "action",    "argument": "TOGGLE"},
        {"interface": "fixed_step"},
        {"interface": "set_state", "max_value": 1},
    ]
    result = G.classify_input("ALPHA_SWITCH", inputs)
    assert result["type"] == G.INPUT_TYPE_SWITCH
    assert result["arg0"] == "0"
    assert result["arg1"] == "1"
    assert result["arg0fast"] is None
    assert result["arg1fast"] is None


def test_multipos_mapping():
    inputs = [
        {"interface": "fixed_step"},
        {"interface": "set_state", "max_value": 2},
    ]
    result = G.classify_input("BETA_MULTIPOS", inputs)
    assert result["type"] == G.INPUT_TYPE_MULTIPOS
    assert result["arg0"] is None
    assert result["arg1"] is None


def test_encoder_mapping():
    inputs = [{"interface": "fixed_step"}]
    result = G.classify_input("DELTA_ENCODER", inputs)
    assert result["type"] == G.INPUT_TYPE_ENCODER
    assert result["arg0"] == "DEC"
    assert result["arg1"] == "INC"


def test_accel_encoder_mapping():
    inputs = [
        {"interface": "fixed_step",
         "argument_decrement": "DEC",  "argument_increment": "INC"},
        {"interface": "fixed_step",
         "argument_decrement": "DEC_FAST", "argument_increment": "INC_FAST"},
    ]
    result = G.classify_input("EPSILON_ACCEL", inputs)
    assert result["type"] == G.INPUT_TYPE_ACCEL_ENCODER
    assert result["arg0"]     == "DEC"
    assert result["arg1"]     == "INC"
    assert result["arg0fast"] == "DEC_FAST"
    assert result["arg1fast"] == "INC_FAST"


def test_action_mapping():
    inputs = [{"interface": "action", "argument": "1"}]
    result = G.classify_input("GAMMA_ACTION", inputs)
    assert result["type"] == G.INPUT_TYPE_ACTION
    assert result["arg0"] == "1"
    assert result["arg1"] is None


def test_analog_mapping():
    inputs = [
        {"interface": "set_state",    "max_value": 65535},
        {"interface": "variable_step","max_value": 65535, "suggested_step": 3200},
    ]
    result = G.classify_input("IOTA_VARSTEP", inputs)
    assert result["type"] == G.INPUT_TYPE_ANALOG
    assert result["arg0"]     is None
    assert result["arg1"]     is None
    assert result["arg0fast"] is None
    assert result["arg1fast"] is None


def test_cover_maps_as_switch():
    inputs = [
        {"interface": "action",    "argument": "TOGGLE"},
        {"interface": "fixed_step"},
        {"interface": "set_state", "max_value": 1},
    ]
    result = G.classify_input("ZETA_COVER", inputs)
    assert result["type"] == G.INPUT_TYPE_SWITCH
    assert result["arg0"] == "0"
    assert result["arg1"] == "1"


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


def test_inputmap_switch_args():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert 'InputType::SWITCH' in content
    assert '"0"' in content
    assert '"1"' in content


def test_inputmap_multipos_nullptrs():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert 'InputType::MULTIPOS' in content


def test_inputmap_analog_type():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert 'InputType::ANALOG' in content


def test_inputmap_accel_encoder_args():
    _, entries, _, _, _ = run_against_fixture()
    content = G.emit_inputmap(entries, "fixture.jsonp", "2025-01-01T00:00:00Z")
    assert 'InputType::ACCEL_ENCODER' in content
    assert '"DEC_FAST"' in content
    assert '"INC_FAST"' in content


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
    input_entries,  input_gaps  = G.build_input_entries(controls)
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
