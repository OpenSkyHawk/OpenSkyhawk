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
| 5 | 3.3 V (chip power) |
| 6 | spare |

LED backlight power is carried on a **separate 2-pin Molex Mini-Fit Jr** connector (not the signal harness) — `+12V_BACKLIGHT` and `BACKLIGHT_SW_RETURN`. Breakout boards with analog outputs or interrupts use an 8-pin JST-XH variant with additional signals on pins 6–8.

**Gauges:** X27.589 Switec stepper motors mounted shaft-through-PCB on the controller board.

**LED backlighting:** LEDs placed on the front side of the PCB; all other components on the back side.

**DCS communication:** Dual-path. RP2040 HID controllers connect directly via USB HID. DCS-BIOS state for cockpit panels routes: PC → USB serial → RP2040 (standalone gateway box) → UART → STM32 gateway node → CAN bus → all avionics nodes.

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

### Waveshare RP2040-Plus — required solder bridges

Two pads are disconnected by default and must be bridged before the board is usable:

| Pad | Purpose | Without it |
|---|---|---|
| **RGB** | NeoPixel data line to GP23 | Onboard WS2812 does not respond |
| **VREF** | ADC reference voltage (to 3.3V) | All ADC reads return garbage / float at 4095 |

Both are small exposed pads on the board edge. Bridge with a solder blob or jumper wire. Do this on every RP2040-Plus before first use.

### USB Identity (HID controllers)

Set at runtime in `setup()` before `Joystick.begin()` — build flags do not work (strings bake into cached framework archive):

```cpp
#include <USB.h>
USB.setManufacturer("OpenSkyhawk");
USB.setProduct("A-4E Skyhawk");
USB.setVIDPID(0x2E8A, 0x4134);
```

**PID allocation table** (VID 0x2E8A = Raspberry Pi, reused for all RP2040 devices):

| Device | Role | PID |
|---|---|---|
| A-4E Skyhawk | DCS-BIOS bridge + flight stick, CDC + HID | `0x4134` |
| A-4E Throttle | HID throttle quadrant | `0x4135` |
| A-4E Rudder | HID rudder pedals | `0x4136` |
| A-4E Button Box | HID button box | `0x4137` |

### Joystick Axes

All flight control axes originate on STM32 sub-nodes collocated with the physical controls. Sub-nodes read sensors locally and send `ControlPacket` structs over CAN → MAIN_NODE → UART → RP2040, which intercepts them and routes to the HID layer. No long analog wire runs needed — only the CAN bus (CANH/CANL/GND) spans the cockpit.

| Axis | HID method | `controlId` | Source | Sub-node |
|---|---|---|---|---|
| Roll | `Joystick.X()` | `CTRL_ROLL` (0x0010) | AS5600 / pot | Stick sub-node |
| Pitch | `Joystick.Y()` | `CTRL_PITCH` (0x0011) | AS5600 / pot | Stick sub-node |
| Throttle | `Joystick.sliderLeft()` | `CTRL_THROTTLE` (0x0012) | ADC pot | Throttle sub-node |
| Rudder | `Joystick.Zrotate()` | `CTRL_RUDDER` (0x0013) | ADC pot | Pedal sub-node |
| Left brake | `Joystick.sliderRight()` | `CTRL_BRAKE_L` (0x0014) | ADC pot | Pedal sub-node |
| Right brake | *(8th axis)* | `CTRL_BRAKE_R` (0x0015) | ADC pot | Pedal sub-node |
| Zoom | `Joystick.Z()` | `CTRL_ZOOM` (0x0016) | ADC pot | Throttle sub-node |

Use `Joystick.use16bit()` + `Joystick.useManualSend(true)` for full ±32767 range and batched HID reports.

### Flight Control Sub-nodes

Three dedicated STM32 sub-nodes handle all physical flight controls:

**Stick sub-node** — mounted in the stick base:
- Roll + Pitch via AS5600 hall-effect sensors (preferred) or pots
- Stick grip buttons (trigger, NWS, etc.) → HID buttons via `CTRL_*` IDs

**Throttle sub-node** — mounted at throttle quadrant:
- Throttle lever + Zoom slider → STM32 ADC
- Throttle grip switches → cockpit panel inputs (DCS-BIOS) or HID buttons

**Pedal sub-node** — mounted under floor/seat:
- Rudder axis + left/right toe brakes → STM32 ADC (3 channels)

STM32F103 has sufficient onboard ADC channels for all three pods. No ADS1115 needed.

### Stick Angle Sensing — AS5600 (preferred over pots)

AS5600 hall effect sensor: 12-bit, contactless, no wear, no dead zones. Fixed I²C address 0x36 — two axes on one sub-node require two I²C buses:
- Roll → I2C0 (PB6/PB7 on STM32)
- Pitch → I2C1 (PB10/PB11 on STM32)

Magnet: diametrically magnetised disc, 6mm dia × 2.5mm thick, 0.5–3mm gap to IC. Mount to be 3D printed (Fusion 360). No ferrous material near the magnet in the mount.

## CAN Avionics Controllers (STM32F103CBT6)

Unchanged from existing architecture. Custom PCBs per controller group. CAN bus primary (SN65HVD230 transceiver on PA11/PA12). No USB at runtime.

## Gateway Role — Standalone RP2040 Gateway Box

A dedicated RP2040 module (not embedded in any flight control) acts as the CAN gateway. It runs two serial interfaces simultaneously:

| Interface | Object | Baud | Purpose |
|-----------|--------|------|---------|
| USB CDC serial | `Serial` | **250000** | DCS-BIOS ↔ PC (fixed by DCS-BIOS protocol) |
| Hardware UART | `Serial1` | **250000** | RP2040 ↔ STM32 master node |

UART baud matches DCS-BIOS protocol rate on both hops — no buffering mismatch.

UART wiring to the CAN gateway STM32 node:

| Signal | RP2040 pin | STM32 pin |
|--------|-----------|-----------|
| TX | GP0 (UART0 TX) | PA3 (UART2 RX) |
| RX | GP1 (UART0 RX) | PA2 (UART2 TX) |
| GND | shared GND | shared GND |

**Use UART2 (PA2/PA3) on STM32, not UART1.** Remapping `Serial` to `Serial1` (PA9/PA10) causes "multiple definition of Serial2" compile errors or runtime failures with STM32duino. With no CDC flag, `Serial` maps natively to UART2 on PA2/PA3 — use that.

Both sides are 3.3V — no level shifter required.

**PlatformIO dependency:** `dcs-skunkworks/DCS-BIOS`. Route to USB with `#define DCSBIOS_DEFAULT_SERIAL` before `#include <DCSBIOS.h>`.

**STM32 USB CDC must not be used for DCS-BIOS.** Under sustained DCS-BIOS data flow the STM32 native USB CDC port crashes (socat exits "Permission denied"; port becomes unusable until replug). Always use UART on STM32 nodes and bridge via an RP2040 module (Tiny2040 or similar). The RP2040's USB stack handles 250000 baud DCS-BIOS traffic without issue. Bridge firmware and wiring: `Firmware/HID_Controllers/DCS_BIOS_Bridge/`.

### Packet Format (RP2040 ↔ STM32)

Simple 4-byte struct — no framing overhead needed at this scale:

```cpp
struct ControlPacket {
    uint16_t controlId;  // panel control identifier
    uint16_t value;      // current value
};
```

### controlId Namespace

`controlId` determines how the RP2040 gateway routes an incoming packet:

| Range | Owner | Routing |
|-------|-------|---------|
| `0x0010` – `0x00FF` | Flight control axes & buttons | HID only — `Joystick.*()` — never DCS-BIOS |
| `0x8000` – `0x86FF` | DCS-BIOS A-4E-C addresses | DCS-BIOS only — `sendDcsBiosMessage()` — never HID |
| `0xFFFF` | Reserved: TEST_SEQ | Triggers TEST_SEQ CAN frame — internal use only |

DCS-BIOS output addresses are used directly as `controlId` values in the `0x8000+` range. This means no translation table is needed for the downstream direction (DCS → cockpit): the master node receives a `ControlPacket` from the RP2040, broadcasts it over CAN, and each sub-node extracts `value` from its own address slot.

**RP2040 routing logic:** `controlId < 0x8000` → HID, `controlId >= 0x8000` → `sendDcsBiosMessage()`.

The A-4E-C mod has no axis exports (stick/rudder positions are HID-only inputs). The two ranges therefore never overlap.

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
RP2040 (standalone gateway box)
 ↕ UART @ 250000 baud  (ControlPacket structs, bidirectional)
STM32F103 (CAN master node)
 ↕ CAN bus @ 500 kbps
 ├── All cockpit avionics nodes  (switches, gauges, lighting)
 ├── Stick sub-node   (AS5600 roll/pitch → CTRL_ROLL/PITCH)
 ├── Throttle sub-node (ADC throttle/zoom → CTRL_THROTTLE/ZOOM)
 └── Pedal sub-node   (ADC rudder/brakes → CTRL_RUDDER/BRAKE_*)
```

**RP2040 routing:** controlId < 0x8000 → `Joystick.*()` HID; controlId ≥ 0x8000 → `sendDcsBiosMessage()`.

**STM32 master node role:** CAN Rx interrupt → pack into `ControlPacket` → push out USART TX to RP2040. Reverse: receive `ControlPacket` from RP2040 → broadcast LED/gauge state over CAN.

**CAN bandwidth:** 500 kbps bus, ~4,500 frames/sec capacity. Worst case (A-4E-C 89 float outputs at 30 Hz, batched 2-per-frame): ~30% utilisation. Sub-node flight control packets add negligible load at human axis rates.

# PCB Architecture

Each physical PCB is its own KiCad project. Controller groups live under a shared parent folder:

```
PCB/<Console>/<ControllerGroup>/
├── <ControllerGroup>_MCU/    ← main board (MCU + panel switches/LEDs merged)
├── <Panel_A>/                ← breakout board, harness to MCU board
└── <Panel_B>/                ← breakout board, harness to MCU board
```

Main boards have LEDs on the front side and all other components on the back side.
