/**
 * @file HIDControls.h
 * @brief CAN controlId constants for HID axes and buttons.
 *
 * Shared between STM32 (via CANProtocol) and RP2040 (SimGateway sketches).
 * Contains only #define constants — no classes, no functions, no state.
 *
 * controlId routing by range:
 *   0x0010–0x001F  HID axes   — routed to Joystick axis setters on SimGateway
 *   0x0020–0x009F  HID buttons — routed to Joystick button setters on SimGateway
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

// ── HID buttons — controlId range 0x0020–0x009F (128 slots) ──────────────────

#define CTRL_TRIGGER    0x0020  // Trigger (button index 0) — stick grip
// 0x0021–0x009F: additional buttons added here as stick/throttle grip is catalogued

// ── Range sentinels (used by CANProtocol and SimGateway for routing checks) ──

#define CTRL_ID_HID_MIN 0x0010  // First valid HID controlId
#define CTRL_ID_HID_MAX 0x009F  // Last valid HID controlId
