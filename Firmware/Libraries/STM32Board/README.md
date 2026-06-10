# STM32Board

Shared STM32F103 hardware initialisation for OpenSkyhawk avionics nodes.

Every OpenSkyhawk CAN avionics board uses the same MCU (STM32F103CBT6) with the same fixed pinout: SN65HVD230 CAN transceiver on PA11/PA12, diagnostic UART on USART1 PA9/PA10, and a bi-color status LED on PB14 (red) / PB15 (green). `STM32Board` defines `HAL_CAN_MspInit` once and provides a consistent API so `PanelGroup` and `PanelBridge` don't duplicate this boilerplate.

## Dependencies

- STM32duino Arduino core (`ststm32` platform)
- STM32 HAL CAN (`-DHAL_CAN_MODULE_ENABLED` build flag)

## Usage

`STM32Board` is not used directly in sketches — it is called by `PanelGroup::setup()`  
and `PanelBridge::setup()`. To enable diagnostic output, call `setDebug(true)` before  
the domain-layer setup function:

```cpp
#include <PanelGroup.h>   // or <PanelBridge.h>

void setup() {
    STM32Board::setDebug(true);   // optional — enables USART1 diagnostic output
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
```

If `setDebug(false)` (the default), the USART1 peripheral is still initialised (negligible cost) but all `print()` calls are skipped — no CPU cycles spent on string formatting or UART transmission.

## CAN timing

500 kbps at 72 MHz system clock:

| Parameter | Value |
|---|---|
| Prescaler | 4 |
| SyncJumpWidth | CAN_SJW_4TQ |
| TimeSeg1 | CAN_BS1_13TQ |
| TimeSeg2 | CAN_BS2_4TQ |
| Bit time | 1 + 13 + 4 = 18 TQ → 72 MHz / 4 / 18 = **500 kbps** |

SJW=4 is required to tolerate the crystal tolerance variation between Blue Pill boards.

## Status LED blink patterns

| Red (PB14) | Green (PB15) | Meaning |
|---|---|---|
| Slow blink 1 Hz | OFF | Booting / initialising |
| OFF | Slow blink 1 Hz | Normal — CAN bus healthy |
| Fast blink 4 Hz | OFF | TEC > 0 — transmit errors accumulating |
| Solid ON | OFF | Bus-off — CAN controller halted |
| Alternating | Alternating | Warning / degraded state |

## API

See [`STM32Board.h`](STM32Board.h) for full Doxygen documentation.

| Function | Description |
|---|---|
| `begin()` | Init PB14/PB15 LEDs, DiagSerial, CAN HAL (no HAL_CAN_Start) |
| `tick()` | Advance LED blink state machine — call every loop() |
| `onCanStatus(CanStatus)` | Update LED state from CAN bus health report |
| `setDebug(bool)` | Enable/disable all diagnostic output |
| `log(msg)` | Print a string if debug is enabled |
| `isDebug()` | Guard multi-field print blocks |
| `diagSerial()` | Access USART1 for formatted output |
| `canHandle()` | Internal — HAL handle for CANProtocol use only |
