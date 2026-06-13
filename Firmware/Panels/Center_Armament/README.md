# Center_Armament Firmware

STM32F103CBT6 CAN avionics controller for the Armament Panel (host) + AWRS
Panel and Misc Switch Panel (breakouts). Communicates with the rest of the
cockpit exclusively over CAN bus — it does not run DCS-BIOS and has no direct
connection to the PC or the RP2040 gateway.

For pinout, I²C addresses, and DCS-BIOS mappings see
`docs/claude/controllers/Center_Armament.md`.

## Development Notes

When prototyping with a **Chinese Blue Pill clone** (STM32F103C8T6), ST-Link
may refuse to connect because the clone reports JTAG ID `0x2ba01477` instead
of the standard `0x1ba01477`. Override it in `platformio.ini`:

```ini
upload_flags =
    -c
    set CPUTAPID 0x2ba01477
debug_extra_cmds = set CPUTAPID 0x2ba01477
```

Full debug session notes (DCS-BIOS, USB CDC issues, UART bridge) are in
`docs/claude/dcsbios-stm32-debug.md` — those findings apply to the STM32 CAN
master node, not to this panel node.
