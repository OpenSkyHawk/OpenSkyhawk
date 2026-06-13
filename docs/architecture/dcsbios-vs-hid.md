# DCS-BIOS vs HID Controls

Every control you add to a panel takes one of two paths to DCS: the **DCS-BIOS path** or the
**HID path**. Choosing correctly is the single most important decision when implementing a
panel — it determines whether the control "just works" or whether every builder has to bind
it by hand. Read this page before you wire up a new control.

!!! tip "The decision rule"
    **Default to DCS-BIOS for everything.** Use HID *only* for momentary controls that have
    no DCS-BIOS address, or for flight-control axes. HID controls require manual binding in
    DCS input settings or a provided profile — that's friction for every builder and kit
    buyer, so don't reach for it unless you have to.

## The two paths

Everything travels the CAN bus in the same `ControlPacket` format. The `controlId` decides
the path, and the routing happens at different tiers:

| Path | controlId range | Routed by | What DCS needs from the user |
|------|-----------------|-----------|------------------------------|
| **DCS-BIOS** | `0x8000`–`0x86FF` | PanelBridge | Nothing beyond running DCS-BIOS export |
| **HID** | `< 0x8000` | SimGateway | Manual input binding or a provided `.lua` profile |

### DCS-BIOS path

For any control that has a DCS-BIOS address. The state is managed by DCS-BIOS itself, and the
control syncs automatically — flip a switch in the pit and it moves in the sim, change it in
the sim and your LED or gauge follows. **Zero DCS setup** beyond running the DCS-BIOS export.

A PanelGroup node fires a compact `DCSIN_*` command ID in the `0x8000`–`0x86FF` range.
PanelBridge looks it up in its generated input map and calls `sendDcsBiosMessage()` with the
real DCS-BIOS command string. The node never knows the command name — only the compact ID.

This is why the entire A-4E-C control set works without any per-control declarations in a
sketch: the generated input map already covers it.

### HID path

For controls with **no** DCS-BIOS address — purely stateless momentary controls, and flight
controls. These are handled entirely at the SimGateway layer as standard joystick axes and
buttons; **PanelBridge never sees them**. A node fires a `controlId` below `0x8000`,
PanelBridge wraps it in a HID frame, and SimGateway applies it to the USB HID report.

## Why HID adds friction

A DCS-BIOS control is bound by name inside the module — it works the moment DCS-BIOS is
running. A HID control is just an anonymous joystick axis or button as far as Windows and DCS
are concerned, which means:

- **Manual binding.** Someone has to open DCS input settings and map each axis and button, or
  import a provided `.lua` profile.
- **Profile maintenance.** That profile has to be kept in step with the firmware's button and
  axis assignments.
- **Platform limits.** DirectInput exposes 8 axes, 128 buttons, and 4 POV hats per device —
  generous, but finite, and shared across the whole cockpit's HID surface.

Multiply that across a cockpit's worth of panels and the DCS-BIOS path is clearly the one you
want by default.

## When HID is the right choice

HID is correct in exactly two situations:

1. **Flight-control axes.** The A-4E-C exposes no axis exports through DCS-BIOS — stick,
   throttle, and rudder are HID-only inputs. HID is mandatory here. This is the main reason
   the HID path exists, and it's primarily for the planned replica HOTAS.
2. **Momentary controls with no DCS-BIOS address.** A purely stateless button (trigger,
   pickle) that the A-4E-C only accepts as a raw input, with no DCS-BIOS command behind it.

The seven planned HID axes are Roll, Pitch, Rudder, Speed Brake, Left Brake, Throttle, and
Zoom — 7 of the 8 DirectInput axis slots.

## The controlId ranges

| Range | Type | Path | Handler |
|-------|------|------|---------|
| `0x0010`–`0x001F` | HID axes | HID | SimGateway → `HIDAxis` |
| `0x0020`–`0x002F` | HID hat switches | HID | SimGateway → `HIDHatSwitch` |
| `0x0030`–`0x00AF` | HID buttons | HID | SimGateway → `HIDButton` |
| `0x00B0`–`0x00FF` | Reserved HID expansion | — | not exposed by the current USB report |
| `0x8000`–`0x86FF` | DCS-BIOS compact IDs (`DCSIN_*`) | DCS-BIOS | PanelBridge → `sendDcsBiosMessage()` |
| `0xFFFF` | Reserved / invalid sentinel | — | dropped |

## Practical examples — the Armament Group

The Center Console Armament Group is almost entirely DCS-BIOS, because nearly every armament
control has a DCS-BIOS address:

- **Armament panel switches** (e.g. the master arm switch) — DCS-BIOS path. A two-position
  switch fires its `DCSIN_*` ID with value 0 or 1; PanelBridge sends the matching DCS-BIOS
  command. No binding needed.
- **AWRS panel** rotary and selector controls, **Misc Switch panel** switches — DCS-BIOS path,
  same pattern, multi-position controls send a position index.
- A **bomb-release / pickle button** *would* use the HID path only if the A-4E-C had no
  DCS-BIOS command for it; if a DCS-BIOS address exists, use it.

The flight stick that eventually mounts in front of this console is the opposite case — its
axes and trigger go through HID, because that's the only path DCS offers for them.

!!! note "Output addresses are a different namespace"
    Don't confuse input `controlId`s with output addresses. Output objects (LEDs, gauges) use
    `A_4E_C_*` address constants and `A_4E_C_*_AM` bitmasks from the generated headers — a
    separate namespace that is never used as an input `controlId`. (And note the generated
    headers omit the old `_A` suffix.)
