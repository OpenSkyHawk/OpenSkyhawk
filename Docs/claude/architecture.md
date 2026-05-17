# Repository Structure

Organized by discipline, then by console position (Left / Center / Right):

- `CAD/` — Fusion 360 source files (`.f3d`). STLs and STEP exports are gitignored; generate from source.
- `PCB/` — KiCad projects. `PCB/<Console>/<Controller>/` holds one KiCad project per physical PCB. `PCB/Libraries/` holds shared symbols and footprints.
- `Firmware/` — PlatformIO projects (preferred) or Arduino sketches. `Firmware/<name>/` — one STM32 CAN avionics controller per folder. `Firmware/HID_Controllers/<name>/` — one RP2040 HID controller per folder. `Firmware/Libraries/` — shared code; STM32 and RP2040 libraries in separate subdirectories.
- `Docs/References/` — cockpit photos, manuals, screenshots. `Docs/Datasheets/` — component datasheets.
- `docs/claude/controllers/` — per-controller reference docs (pinout, I²C addresses, CAN IDs).

# Firmware Architecture

Each folder under `Firmware/` maps to one STM32 MCU board. Controllers communicate over CAN bus. A controller may drive one panel or a group of adjacent panels.

Controllers fall into two categories: **HID controllers** (RP2040, direct USB to PC) and **CAN avionics controllers** (STM32F103CBT6, CAN bus). The RP2040 in the flight stick doubles as the CAN gateway, bridging USB serial from the PC to the CAN network via UART.

**Toolchain:** PlatformIO preferred (`platformio.ini` + `src/main.cpp`). Arduino IDE + STM32duino is an acceptable fallback (`.ino` file).

**I/O expansion:** MCP23017 (I²C, up to 8 per bus at addresses 0x20–0x27, 16 GPIO each)

**Analog inputs:** Resistor-ladder rotary selectors and pots read via STM32 ADC or I²C ADC (ADS1115) on breakout boards.

**Inter-board harness:** Breakout panel boards connect to their parent controller via a standardised 6-pin JST-XH harness:

| Pin | Signal |
|---|---|
| 1 | SDA |
| 2 | SCL |
| 3 | GND |
| 4 | GND |
| 5 | 12 V switched (LED backlight — MOSFET on MCU board, PWM-controlled) |
| 6 | 3.3 V (chip power) |

**Gauges:** X27.589 Switec stepper motors mounted shaft-through-PCB on the controller board.

**LED backlighting:** LEDs placed on the front side of the PCB; all other components on the back side.

**DCS communication:** Dual-path. RP2040 HID controllers connect directly via USB HID. DCS-BIOS state for cockpit panels routes: PC → USB serial → RP2040 (flight stick, gateway role) → UART → STM32 gateway node → CAN bus → all avionics nodes.

**Naming convention:** Functional names from the start — `Center_Armament`, `Left_ECM`, etc. No `Controller_NN` placeholders.

**Licensing:** GPL v2 (`Firmware/LICENSE`) due to DCS-BIOS dependency (if used).

# Dual-MCU Architecture

## MCU Roles

| Category | MCU | Role |
|----------|-----|------|
| HID Controllers | RP2040 module | Flight controls, button boxes — USB HID to PC |
| CAN Avionics Controllers | STM32F103CBT6 | Cockpit panels — switches, gauges, lighting |

## HID Controllers (RP2040)

Off-the-shelf modules only — no custom PCB.

Preferred modules: Raspberry Pi Pico 2, Raspberry Pi Pico, Tiny2040, RP2040 Zero. Choose by mechanical fit; all are functionally equivalent for HID use.

Devices: flight stick, throttle quadrant, rudder pedals, standalone button boxes.

Toolchain: PlatformIO (earlephilhower/arduino-pico platform) or Arduino IDE with Arduino-Pico core. TinyUSB for USB HID + CDC composite device.

RP2040 modules are bus-powered from USB. No 12V supply needed.

## CAN Avionics Controllers (STM32F103CBT6)

Unchanged from existing architecture. Custom PCBs per controller group. CAN bus primary (SN65HVD230 transceiver on PA11/PA12). No USB at runtime.

## Gateway Role — Flight Stick RP2040

The flight stick's RP2040 runs two serial interfaces simultaneously:

| Interface | Object | Baud | Purpose |
|-----------|--------|------|---------|
| USB CDC serial | `Serial` | **250000** | DCS-BIOS ↔ PC (fixed by DCS-BIOS protocol) |
| Hardware UART | `Serial1` | 115200+ | RP2040 ↔ STM32 master node |

UART wiring to the CAN gateway STM32 node:

| Signal | RP2040 pin | STM32 pin |
|--------|-----------|-----------|
| TX | RP2040 TX | PA9 (USART1 RX) |
| RX | RP2040 RX | PA10 (USART1 TX) |
| GND | shared GND | shared GND |

Both sides are 3.3V — no level shifter required.

**PlatformIO dependency:** `dcs-skunkworks/DCS-BIOS`. Route to USB with `#define DCSBIOS_DEFAULT_SERIAL` before `#include <DCSBIOS.h>`.

### Packet Format (RP2040 ↔ STM32)

Simple 4-byte struct — no framing overhead needed at this scale:

```cpp
struct ControlPacket {
    uint16_t controlId;  // panel control identifier
    uint16_t value;      // current value
};
```

### DCS-BIOS Integration Constraint

DCS-BIOS is designed to read hardware state directly from the MCU it runs on — it cannot consume CAN bus packets natively. On the gateway RP2040, UART packets from the STM32 cluster must be parsed manually and translated into DCS-BIOS commands via `sendDcsBiosMessage("CONTROL_NAME", "value")`.

```cpp
// Cockpit → DCS (switch/axis input from STM32)
switch (packet.controlId) {
    case 0x0101:
        sendDcsBiosMessage("MASTER_CAUTION_SW", packet.value ? "1" : "0");
        break;
}
```

### Bidirectional Flow — DCS → Cockpit

LED indicator states and gauge targets flow in the opposite direction: DCS-BIOS outputs are intercepted on the RP2040 using `DcsBios::StringBuffer` (or integer buffer) callbacks and pushed back down `Serial1` to the STM32 master, which distributes them over CAN.

## Full DCS-BIOS Message Path

```
PC
 ↕ USB HID  (joystick axes, buttons — latency-critical)
 ↕ USB CDC serial @ 250000 baud  (DCS-BIOS cockpit state)
RP2040 (flight stick + gateway)
 ↕ UART @ 115200+  (ControlPacket structs, bidirectional)
STM32F103 (CAN master node)
 ↕ CAN bus
All cockpit avionics nodes
```

**STM32 master node role:** CAN Rx interrupt → pack into `ControlPacket` → push out USART TX to RP2040. Reverse: receive `ControlPacket` from RP2040 → broadcast LED/gauge state over CAN.

# PCB Architecture

Each physical PCB is its own KiCad project. Controller groups live under a shared parent folder:

```
PCB/<Console>/<ControllerGroup>/
├── <ControllerGroup>_MCU/    ← main board (MCU + panel switches/LEDs merged)
├── <Panel_A>/                ← breakout board, harness to MCU board
└── <Panel_B>/                ← breakout board, harness to MCU board
```

Main boards have LEDs on the front side and all other components on the back side.
