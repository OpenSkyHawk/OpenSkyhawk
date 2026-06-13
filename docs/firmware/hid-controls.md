# HID Controls

Some controls don't go through DCS-BIOS — flight-control axes, and momentary controls with no
DCS-BIOS address. These take the **HID path**: they appear to the PC as a standard USB joystick,
handled entirely by [SimGateway](../architecture/sim-gateway.md). PanelBridge never sees them.
This page is the `controlId` reference for that path. For *when* to use HID vs DCS-BIOS, see
[DCS-BIOS vs HID](../architecture/dcsbios-vs-hid.md).

## controlId ranges

HID controls use `controlId` values **below `0x8000`**, allocated in `HIDControls.h`:

| Range | Type | Handler | USB capacity |
|-------|------|---------|--------------|
| `0x0010`–`0x001F` | Axes | `HIDAxis` | 8 axes |
| `0x0020`–`0x002F` | Hat switches | `HIDHatSwitch` | 4 hats |
| `0x0030`–`0x00AF` | Buttons | `HIDButton` | 128 buttons |
| `0x00B0`–`0x00FF` | Reserved HID expansion | — | not exposed |

`CTRL_ID_HID_MIN = 0x0010`, `CTRL_ID_HID_MAX = 0x00FF`.

## The CTRL_* constants

From `Firmware/Libraries/HIDControls/HIDControls.h` — the authoritative source:

| Constant | Value | Control |
|----------|-------|---------|
| `CTRL_ROLL` | `0x0010` | Roll axis — stick sub-node |
| `CTRL_PITCH` | `0x0011` | Pitch axis — stick sub-node |
| `CTRL_THROTTLE` | `0x0012` | Throttle lever — throttle sub-node |
| `CTRL_RUDDER` | `0x0013` | Rudder axis — pedal sub-node |
| `CTRL_BRAKE_L` | `0x0014` | Left toe brake — pedal sub-node |
| `CTRL_BRAKE_R` | `0x0015` | Right toe brake — pedal sub-node |
| `CTRL_ZOOM` | `0x0016` | Zoom axis |
| `CTRL_HAT_0` | `0x0020` | Hat switch 0 — stick grip |
| `CTRL_TRIGGER` | `0x0030` | Trigger — button index 0 |

That's 7 of the 8 available axes — the planned out-of-console HOTAS allocation (stick, rudder
pedals, replica throttle). The throttle *panel* is a separate Left Console DCS-BIOS panel; only
the throttle *axis* is HID. The speed brake is a DCS-BIOS switch, not an axis.

## How HID controls are declared

Unlike DCS-BIOS controls (which need no sketch declaration — the input map covers them), HID
controls are declared on the **SimGateway** sketch, binding a `controlId` to a joystick slot:

```cpp
OpenSkyhawk::HIDAxis   roll   (CTRL_ROLL,    0);  // axis index 0
OpenSkyhawk::HIDAxis   throttle(CTRL_THROTTLE, 2);
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER,  0);  // button index 0
OpenSkyhawk::HIDHatSwitch hat (CTRL_HAT_0,    0);  // hat index 0
```

The matching PanelGroup input fires the same `controlId`; SimGateway maps it to the joystick
report:

- **`HIDAxis`** — incoming 0–65535 mapped to ±32767 (`value − 32768`)
- **`HIDButton`** — `value != 0` → pressed
- **`HIDHatSwitch`** — direction nibble (0 = centered, 1–8 = N…NW)

After draining all HID frames in a loop, SimGateway calls `OsJoystick.send()` once. The USB HID
profile is 8 axes / 128 buttons / 4 hats — see [SimGateway](../architecture/sim-gateway.md) for
the descriptor and platform limits.
