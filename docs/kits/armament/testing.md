---
status: planned
---

# Armament Group Kit — Testing & Troubleshooting

!!! warning "Planned kit — not yet available"
    Final test steps and troubleshooting ship with the kit, informed by end-to-end validation.
    Details below are **TBD**.

## Verify it works

With the [DCS setup](dcs-setup.md) done and a mission running in the A-4E-C:

1. **Status light** — the controller's status LED should be **blinking green** (CAN bus
   healthy). *(photo TBD)*
2. **Inputs** — flip each armament switch and confirm the matching control moves in the cockpit.
3. **Outputs** — trigger the relevant sim states and confirm the panel's indicators / lights
   follow.

## Troubleshooting (planned)

| Symptom | First thing to check |
|---------|----------------------|
| Status light solid red | CAN bus / termination — is the panel connected to the SimGateway + PanelBridge backbone? |
| Nothing happens in DCS | Is DCS-BIOS export running? Is the USB cable connected? |
| Some controls work, others don't | Connector seated fully? Check the JST-XH harness to that breakout |
| No power / no lights | Power harness orientation; is the bus supplying 12 V / 5 V? |

For deeper diagnostics (DiagSerial, error counters, the status-LED states), the builder
reference is [Bring-Up & Testing](../../guides/bring-up.md) — but a kit buyer shouldn't normally
need it.

!!! note "Help improve this"
    Once kits are in the wild, real failure cases will be folded into this table. If you hit
    something, report it — see [How to Contribute](../../contributing/index.md).
