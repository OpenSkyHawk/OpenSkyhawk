#pragma once
#include <stdint.h>

// ── Flight control axis IDs (0x0010–0x001F) ───────────────────────────────────
// Read by STM32 sub-nodes (AS5600 / ADC), sent over CAN, routed to
// Joystick.*() on RP2040. Never forwarded to DCS-BIOS.

static constexpr uint16_t CTRL_ROLL     = 0x0010;  // Joystick.X()
static constexpr uint16_t CTRL_PITCH    = 0x0011;  // Joystick.Y()
static constexpr uint16_t CTRL_THROTTLE = 0x0012;  // Joystick.sliderLeft()
static constexpr uint16_t CTRL_RUDDER   = 0x0013;  // Joystick.Zrotate()
static constexpr uint16_t CTRL_BRAKE_L  = 0x0014;  // Joystick.sliderRight()
static constexpr uint16_t CTRL_BRAKE_R  = 0x0015;  // 8th axis
static constexpr uint16_t CTRL_ZOOM     = 0x0016;  // Joystick.Z()
