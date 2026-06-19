/**
 * @file A4EC_DrumReadouts.h
 * @brief A-4E-specific DrumReadout descriptor tables for OpenSkyhawk::DrumDisplay.
 *
 * @details One `static const DrumReadout` per cockpit drum readout, populated entirely from
 * the A_4E_C_* identifiers and A_4E_C_*_AM masks in A4EC_OutputIds.h — never raw hex. The
 * DrumDisplay class (PanelGroup library) is generic; these tables map its descriptor model
 * onto the A-4E's DCS-BIOS output addresses. Header-only data — include it from a display
 * sketch (which already depends on both A4EC and PanelGroup).
 *
 * Several encodings are assumptions flagged TODO(bench): the hemisphere flag dual-role on the
 * rightmost LAT/LON/MAGVAR source, the ARC-51 manual-frequency selector split, and the decimal
 * positions. Confirm on the bench / DCS Model Viewer before treating them as verified.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Outputs/DrumDisplay.h>   // DrumReadout / DrumSource / DrumGlyph / DrumFlag / enums
#include "A4EC_OutputIds.h"        // A_4E_C_* addresses + _AM masks

namespace OpenSkyhawk {

// Shared geometry defaults (mm) — measured drum-window aperture from the feasibility study.
// digit 4.5 x 8.0 mm, 1.0 mm inter-digit gap, flag cell 5.5 mm, decimal glyph 1.8 mm.

// ── APN-153 ground speed — 3 x 1-digit, no flag (bring-up readout, no TODOs) ───
static const DrumSource APN153_SPEED_SRC[] = {
    { A_4E_C_APN153_SPEED_X00, A_4E_C_APN153_SPEED_X00_AM, 1, 2 },  // hundreds → place 2
    { A_4E_C_APN153_SPEED_0X0, A_4E_C_APN153_SPEED_0X0_AM, 1, 1 },  // tens     → place 1
    { A_4E_C_APN153_SPEED_00X, A_4E_C_APN153_SPEED_00X_AM, 1, 0 },  // ones     → place 0
};
static const DrumReadout APN153_SPEED = {
    APN153_SPEED_SRC, 3, /*nDigits*/ 3,
    4.5f, 8.0f, 1.0f, /*groupGapMm*/ 0.0f, /*groupSize*/ 0,
    /*glyphs*/ nullptr, 0,
    /*flag*/ { false, 0, 0, nullptr, 0, 0.0f },
    DrumScroll::SNAP_SETTLE, /*snapThreshold*/ 3.0f,
};

// ── Current latitude — 5 digits + N/S hemisphere flag ─────────────────────────
static const DrumSource NAV_LAT_SRC[] = {
    { A_4E_C_NAV_CURPOS_LAT_X0000, A_4E_C_NAV_CURPOS_LAT_X0000_AM, 1, 4 },
    { A_4E_C_NAV_CURPOS_LAT_0X000, A_4E_C_NAV_CURPOS_LAT_0X000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LAT_00X00, A_4E_C_NAV_CURPOS_LAT_00X00_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LAT_000X0, A_4E_C_NAV_CURPOS_LAT_000X0_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LAT_0000X, A_4E_C_NAV_CURPOS_LAT_0000X_AM, 1, 0 },
};
static const DrumReadout NAV_CURPOS_LAT = {
    NAV_LAT_SRC, 5, /*nDigits*/ 5,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    nullptr, 0,
    // TODO(bench): the rightmost source (_0000X) is labelled the ones digit but the decode note
    // treats it as the N/S hemisphere. Listed as both a digit source (above) AND the flag source
    // (below) — confirm digit vs flag vs both, and the value→face map, on the bench.
    /*flag*/ { true, A_4E_C_NAV_CURPOS_LAT_0000X, A_4E_C_NAV_CURPOS_LAT_0000X_AM, "NS", /*atVisualCol*/ 5, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};

// ── Current longitude — 6 digits + E/W hemisphere flag ────────────────────────
static const DrumSource NAV_LON_SRC[] = {
    { A_4E_C_NAV_CURPOS_LON_X00000, A_4E_C_NAV_CURPOS_LON_X00000_AM, 1, 5 },
    { A_4E_C_NAV_CURPOS_LON_0X0000, A_4E_C_NAV_CURPOS_LON_0X0000_AM, 1, 4 },
    { A_4E_C_NAV_CURPOS_LON_00X000, A_4E_C_NAV_CURPOS_LON_00X000_AM, 1, 3 },
    { A_4E_C_NAV_CURPOS_LON_000X00, A_4E_C_NAV_CURPOS_LON_000X00_AM, 1, 2 },
    { A_4E_C_NAV_CURPOS_LON_0000X0, A_4E_C_NAV_CURPOS_LON_0000X0_AM, 1, 1 },
    { A_4E_C_NAV_CURPOS_LON_00000X, A_4E_C_NAV_CURPOS_LON_00000X_AM, 1, 0 },
};
static const DrumReadout NAV_CURPOS_LON = {
    NAV_LON_SRC, 6, /*nDigits*/ 6,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    nullptr, 0,
    // TODO(bench): same hemisphere dual-role as LAT — _00000X carries the ones digit and/or E/W.
    /*flag*/ { true, A_4E_C_NAV_CURPOS_LON_00000X, A_4E_C_NAV_CURPOS_LON_00000X_AM, "EW", /*atVisualCol*/ 6, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};

// ── Magnetic variation — 5 digits + E/W flag ──────────────────────────────────
static const DrumSource MAGVAR_SRC[] = {
    { A_4E_C_ASN41_MAGVAR_X0000, A_4E_C_ASN41_MAGVAR_X0000_AM, 1, 4 },
    { A_4E_C_ASN41_MAGVAR_0X000, A_4E_C_ASN41_MAGVAR_0X000_AM, 1, 3 },
    { A_4E_C_ASN41_MAGVAR_00X00, A_4E_C_ASN41_MAGVAR_00X00_AM, 1, 2 },
    { A_4E_C_ASN41_MAGVAR_000X0, A_4E_C_ASN41_MAGVAR_000X0_AM, 1, 1 },
    { A_4E_C_ASN41_MAGVAR_0000X, A_4E_C_ASN41_MAGVAR_0000X_AM, 1, 0 },
};
static const DrumReadout ASN41_MAGVAR = {
    MAGVAR_SRC, 5, /*nDigits*/ 5,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    nullptr, 0,
    // TODO(bench): hemisphere dual-role on _0000X, as LAT/LON.
    /*flag*/ { true, A_4E_C_ASN41_MAGVAR_0000X, A_4E_C_ASN41_MAGVAR_0000X_AM, "EW", /*atVisualCol*/ 5, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};

// ── Altimeter (Kollsman) setting — 4 digits, grouped 2+1+1, with a decimal point ─
// NN00 carries the top two digits (places 3,2); 00N0 the tens (place 1); 000N the ones (place 0).
static const DrumSource ALT_ADJ_SRC[] = {
    { A_4E_C_ALT_ADJ_NN00, A_4E_C_ALT_ADJ_NN00_AM, 2, 2 },  // "NN" → places 3,2
    { A_4E_C_ALT_ADJ_00N0, A_4E_C_ALT_ADJ_00N0_AM, 1, 1 },  // tens → place 1
    { A_4E_C_ALT_ADJ_000N, A_4E_C_ALT_ADJ_000N_AM, 1, 0 },  // ones → place 0
};
static const DrumGlyph ALT_ADJ_DOT[] = {
    { '.', /*afterCol*/ 2, /*widthMm*/ 1.8f },  // 29.92 — dot after the 2nd digit from the left
};
static const DrumReadout ALT_ADJ = {
    ALT_ADJ_SRC, 3, /*nDigits*/ 4,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    ALT_ADJ_DOT, 1,
    /*flag*/ { false, 0, 0, nullptr, 0, 0.0f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};
// TODO(bench): confirm NN00 packs two digits 00–99 linearly and the dot sits at "29.92".

// ── ARC-51 displayed frequency — 5 digits (2+1+2) with a decimal ──────────────
static const DrumSource ARC51_FREQ_SRC[] = {
    { A_4E_C_ARC51_FREQ_XX000, A_4E_C_ARC51_FREQ_XX000_AM, 2, 3 },  // "XX" → places 4,3
    { A_4E_C_ARC51_FREQ_00X00, A_4E_C_ARC51_FREQ_00X00_AM, 1, 2 },  // hundreds → place 2
    { A_4E_C_ARC51_FREQ_000XX, A_4E_C_ARC51_FREQ_000XX_AM, 2, 0 },  // "XX" → places 1,0
};
static const DrumGlyph ARC51_FREQ_DOT[] = {
    { '.', /*afterCol*/ 3, /*widthMm*/ 1.8f },  // 225.50 — dot after the 3rd digit from the left
};
static const DrumReadout ARC51_FREQ = {
    ARC51_FREQ_SRC, 3, /*nDigits*/ 5,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    ARC51_FREQ_DOT, 1,
    /*flag*/ { false, 0, 0, nullptr, 0, 0.0f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};
// TODO(bench): confirm the grouped 2-digits-per-source (00–99 linear) encoding and dot position.
// 220–399 MHz needs no special math on the OLED — each digit decodes from its own source.

// ── ARC-51 manual frequency from the 10/1/50 selectors (bit-packed, shared address) ─
// 10 MHz and 1 MHz both live at A_4E_C_ARC51_FREQ_10MHZ/_1MHZ (same address, mask-separated);
// 50 kHz at _50KHZ. The masks split the fields; placement is a best-effort guess.
static const DrumSource ARC51_MANUAL_SRC[] = {
    { A_4E_C_ARC51_FREQ_10MHZ, A_4E_C_ARC51_FREQ_10MHZ_AM, 1, 2 },  // 10 MHz digit → place 2
    { A_4E_C_ARC51_FREQ_1MHZ,  A_4E_C_ARC51_FREQ_1MHZ_AM,  1, 1 },  // 1 MHz digit  → place 1
    { A_4E_C_ARC51_FREQ_50KHZ, A_4E_C_ARC51_FREQ_50KHZ_AM, 1, 0 },  // 50 kHz step  → place 0
};
static const DrumReadout ARC51_FREQ_MANUAL = {
    ARC51_MANUAL_SRC, 3, /*nDigits*/ 3,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    nullptr, 0,
    /*flag*/ { false, 0, 0, nullptr, 0, 0.0f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};
// TODO(bench): the selector fields are bit-packed and two share one address. decodeDigits scales
// the masked field linearly — confirm each field is low-justified within its mask, the per-field
// ranges (10 MHz, 1 MHz, 50 kHz step), and the digit mapping on the bench before trusting this.

// ── BDHI DME range — 3 digits + a real 2-state DME flag ───────────────────────
static const DrumSource BDHI_DME_SRC[] = {
    { A_4E_C_BDHI_DME_X00, A_4E_C_BDHI_DME_X00_AM, 1, 2 },
    { A_4E_C_BDHI_DME_0X0, A_4E_C_BDHI_DME_0X0_AM, 1, 1 },
    { A_4E_C_BDHI_DME_00X, A_4E_C_BDHI_DME_00X_AM, 1, 0 },
};
static const DrumReadout BDHI_DME = {
    BDHI_DME_SRC, 3, /*nDigits*/ 3,
    4.5f, 8.0f, 1.0f, 0.0f, 0,
    nullptr, 0,
    // BDHI DME validity flag (off-flag): 2-state, driven by its own address — a genuine flag source
    // (not a hemisphere dual-role). Faces are placeholders; confirm the off/on rendering on the bench.
    /*flag*/ { true, A_4E_C_BDHI_DME_FLAG, A_4E_C_BDHI_DME_FLAG_AM, " M", /*atVisualCol*/ 3, 5.5f },
    DrumScroll::SNAP_SETTLE, 3.0f,
};
// TODO(bench): confirm the DME flag value→face encoding and choose the off/on glyphs.

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
