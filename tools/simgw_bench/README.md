# simgw_bench — SimGateway link-health bench helper

Small pyserial tool for the Gateway's CDC port (`A-4E Skyhawk DCS-BIOS`), written for
the USB lock-up investigation (#355). It reads the firmware's `DIAG` counters and runs
the "host holds the port but stops reading" stall test.

The message types and the `DIAG` field layout are parsed from
`Firmware/Libraries/SimGateway/AxisCal.h` at run time, so the script never restates a
wire constant and cannot drift from the firmware it ships with.

Close SkyHawkClient first, because only one program can hold the port. Requires `pyserial`.

## Commands

```bash
# Print the counters: UART error bits, dropped lines, recovery actions, event times.
tools/simgw_bench/simgw_bench.py diag /dev/cu.usbmodem1101

# Hold DTR, keep requesting the node roster, never read for 60 s; then check the
# device still answers and print the counters.
tools/simgw_bench/simgw_bench.py stall /dev/cu.usbmodem1101 --seconds 60
```

## Reading the counters

| Field | Points at |
|---|---|
| `UART_OVERRUN` | the UART interrupt itself was held off (e.g. a flash erase, which is excluded) |
| `UART_FRAMING` / `UART_BREAK` | electrical: PanelBridge reset or power-off, noise, a back-feed |
| `UART_RX_OVERFLOW` | `loop()` fell behind the UART |
| `LINES_DROPPED` | the host held the port but stopped reading |
| `LINES_CORRUPT` | non-ASCII bytes on the PanelBridge→host path (line noise) |
| `FIRST_UART_ERR_MS` vs `FIRST_USB_LOSS_MS` | which came first, the UART fault or the USB loss |
| `USB_REATTACHES` / `RECOVERY_REBOOTS` | the recovery ladder ran |
| `RESET_REASON` | `POWER_ON`, `WATCHDOG` (loop hung), or `RECOVERY` (ladder reboot) |

The two counters marked "survives reboot" are kept in watchdog scratch registers. They
persist through a watchdog or recovery reboot and reset on power-off, which on the Gateway
means a USB replug.
