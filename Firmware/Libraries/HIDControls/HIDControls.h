/**
 * @file HIDControls.h
 * @brief CAN controlId constants for HID axes and buttons.
 *
 * Shared between STM32 (via CANProtocol) and RP2040 (SimGateway sketches).
 * Contains only `#define` constants — no classes, no functions, no state.
 *
 * controlId routing by range:
 *   0x0010–0x001F  HID axes        — routed to axis setters on SimGateway
 *   0x0020–0x002F  HID hat switches — routed to hat setters on SimGateway
 *   0x0030–0x00AF  HID buttons     — routed to button setters on SimGateway
 *   0x00B0–0x00FF  Reserved HID expansion slots
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once

// ── HID axes — controlId range 0x0010–0x001F (16 slots) ──────────────────────

#define CTRL_ROLL       0x0010  // Roll axis       — stick sub-node (AS5600 / pot)
#define CTRL_PITCH      0x0011  // Pitch axis      — stick sub-node (AS5600 / pot)
#define CTRL_THROTTLE   0x0012  // Throttle lever  — throttle sub-node (ADC)
#define CTRL_RUDDER     0x0013  // Rudder axis     — pedal sub-node (ADC)
#define CTRL_BRAKE_L    0x0014  // Left toe brake  — pedal sub-node (ADC)
#define CTRL_BRAKE_R    0x0015  // Right toe brake — pedal sub-node (ADC)
#define CTRL_ZOOM       0x0016  // Zoom axis       — throttle sub-node (ADC)
// 0x0017–0x001F: reserved for future axes

// ── HID hat switches — controlId range 0x0020–0x002F (16 slots) ──────────────
// value: 0 = centered, 1 = N, 2 = NE, 3 = E, 4 = SE, 5 = S, 6 = SW, 7 = W, 8 = NW

#define CTRL_HAT_0      0x0020  // Hat switch 0    — stick grip (4-way / 8-way)
// 0x0021–0x002F: reserved for additional hat switches

// ── HID buttons — controlId range 0x0030–0x00AF (128 slots) ──────────────────

#define CTRL_TRIGGER    0x0030  // Trigger (button index 0) — stick grip
// 0x0031–0x00AF: additional buttons added here as stick/throttle grip is catalogued
// 0x00B0–0x00FF: reserved for future HID expansion beyond the current USB report

// ── Range sentinels (used by CANProtocol and SimGateway for routing checks) ──

#define CTRL_ID_HID_MIN 0x0010  // First valid HID controlId
#define CTRL_ID_HID_MAX 0x00FF  // Last reserved HID controlId

