# Video script — First Setup & Updates (#380)

A screen + bench walkthrough of `docs/guides/first-setup.md`: flash every board once, then update
the Gateway from SkyHawkClient. The rule it follows is `Firmware/ScratchPad/FirmwarePlan/12-bootloader.md`
§ Rollout; client copy is from SkyHawkClient `main` `ca96094` (`src/renderer/src/nodes/firmware-view.ts`,
`src/renderer/src/update/update-view.ts`). Re-check both before filming.

**Narration rules.** Say what the viewer does and sees, never how the bytes travel: no "CAN",
"USART2", "serial", "over the bus". Board names as the viewer knows them: the Gateway (its *USB side*
and *cockpit side*), a PDU, a PanelGroup board. Client labels exactly as the client shows them.

**Scope.** Only what works today, the same as the page. Updating PDUs and PanelGroup boards from the
client waits for SkyHawkClient#175 / #176 / #177 — see [Held scenes](#held-scenes).

## Kit on the bench

- Gateway, one PDU, one PanelGroup board with a panel sketch, wired as a small cockpit, on the
  cockpit power supply.
- ST-Link V2 + SWD lead; USB cable for the Gateway.
- PC with SkyHawkClient, STM32CubeProgrammer, PlatformIO, and the release zip
  `openskyhawk-firmware-<version>.zip` unpacked on the desktop.
- Camera on the boards' status LEDs (close enough to tell red + green together from amber).

## Board prep (before rolling)

| For scene | Prepare |
|---|---|
| 3–6 | Boards blank (fully erased) so the first flash is real. |
| 7 | Optional inset: a cockpit side running a build from before the bootloader that still reports its version — the client shows **MANUAL UPDATE ONLY** only for a row that reports a version with bootloader 0 (SkyHawkClient `src/main/gateway/firmware.ts` `noBootloader`). Check which build does that on the bench; skip the inset if none. Re-flash its `-full.hex` afterwards. |
| 8–9 | The Gateway one release behind (previous release's `-full.hex` + `.uf2`), so both rows show **UPDATE AVAILABLE**. |
| 12 | A PanelGroup build that resets in `setup()` before it reports healthy — three boots and it shows **WON'T START**. Keep a good build ready for the fix. |
| 13 | Optional: a cockpit-side build that never takes the update request. Skip the scene if none is ready. |

## Scenes

| # | Shot | Narration | On screen / LEDs |
|---|---|---|---|
| 1 | Bench wide, then the PC | "Every board gets its firmware by hand once. After that, the Gateway updates from SkyHawkClient with the cockpit wired up as it is. Here's both, in order." | Title card *First Setup & Updates*. |
| 2 | Screen: the unpacked zip | "A release is one zip with a folder per board. For the first setup each board needs one file. The files ending in `-full.hex` hold the bootloader and the firmware together — that's what a new board needs. There's no PanelGroup file: every panel group has its own node number and controls, so you build that one from your own sketch." | Folders `gateway/`, `pdu-left/`, `pdu-right/`, `pdu-center/`. Highlight `gateway-bridge-<v>-full.hex`, `gateway-<v>.uf2`, `pdu-left-<v>-full.hex`. |
| 3 | Bench close: ST-Link onto the Gateway's programming header. Screen: STM32CubeProgrammer | "Start with the Gateway's cockpit side. Connect the ST-Link and open `gateway-bridge-…-full.hex` — the flash address is already in the file — and download it." | CubeProgrammer: open file, *Download*, success. Gateway STM32 LED: red blinking (booting) → green blinking. |
| 4 | Screen + bench: plug the Gateway's USB in | "Now the USB side. Close SkyHawkClient, then plug the Gateway into the PC. A new Gateway shows up as a drive called `RPI-RP2` by itself. Drag `gateway-….uf2` onto it; the drive disappears as the Gateway restarts. Only if the drive never appears, hold BOOTSEL while you plug it in." | Finder/Explorer: `RPI-RP2` appears, drag, drive vanishes. Gateway USB-side LEDs: red slow → green slow (connected, no sim data). |
| 5 | Bench: ST-Link onto the PDU | "Each PDU gets its own `-full.hex` the same way. The file already carries the PDU's node number." | CubeProgrammer success; PDU LED red blinking → green blinking. |
| 6 | Screen: terminal in the sketch project, ST-Link on the PanelGroup board | "A PanelGroup board takes your own sketch. With the ST-Link connected, run `pio run -t bootstrap` in the sketch's project: it builds the bootloader and your firmware and flashes both in one go." | Terminal: `pio run -t bootstrap`, both uploads succeed. Board LED red blinking → green blinking. |
| 7 | Screen: SkyHawkClient, **Cockpit** tab → **Firmware** | "Power the cockpit, connect the Gateway and open SkyHawkClient. On the Cockpit tab, the Firmware button lists every board's firmware against the latest release, starting with the Gateway's USB side and its cockpit side. If the cockpit side says *Manual update only*, it has no bootloader yet — flash its `-full.hex` as we just did." | Rows *UP TO DATE*. Cut to the prepped board: **MANUAL UPDATE ONLY** — *"… · no bootloader — update it with a programmer"*. |
| 8 | Screen: Firmware sheet → **Update** on the USB side | "To update, the Gateway goes first, and its USB side before its cockpit side: the USB side's firmware carries the cockpit side's. Click Update on the USB side. The dialog checks a few things first — the relay is stopped, the Gateway is connected, the firmware is on this computer — and offers a button for anything that isn't ready. Keep the PC awake and the cable in until it's done." | Row **UPDATE AVAILABLE**. Checks: *DCS relay stopped*, *Gateway connected*, *Firmware <v> · saved on this computer, checked*. Note *Gateway — it restarts to update and reconnects by itself. Keep it plugged in.* Steps: *Restarting the Gateway to update it* → *Copying the firmware* → *Reconnecting* → *Update finished*. |
| 9 | Screen, then bench close on the LEDs | "Then the cockpit side. Its new firmware came along with the USB side's, so click Update on it too. The whole cockpit pauses for a few seconds while it updates — the other boards' warning LEDs light briefly, and that's expected." | Row sub *"<old> → <v> · carried by the USB side"*. Note **The whole cockpit pauses**. Steps: *Staging the image on the Gateway* → *Entering its bootloader* → *Writing* → *Verifying* → *Restarting* → *Confirming the version it reports*. Gateway STM32 LED: red + green flickering together while the image arrives. PDU / PanelGroup LEDs: amber flicker, then back to green. Both rows **UP TO DATE**. |
| 9b | Screen (optional) | "Start with the cockpit side while the USB side is behind, and the client takes you to the USB side first." | Cockpit-side row *"… · update the USB side first"*. |
| 10 | Screen: firmware sheet | "If the cable comes out partway, plug it back in and check the list. A board still on its old version, or waiting in its bootloader, just needs the update again." | No pull on camera — narration over the sheet. |
| 11 | Bench: ST-Link on the PDU; screen: terminal | "The other boards update by ST-Link for now. A PDU takes the `.elf` from its release folder; a PanelGroup board takes `pio run -t upload` from its sketch. Both keep the bootloader. To move a board to another node number, flash it with that node's firmware — the board takes its number from the firmware." | CubeProgrammer with `pdu-left-<v>.elf`; terminal `pio run -t upload`. |
| 12 | Bench close on LEDs, then screen | "Red and green lit together means a board is in its bootloader, not running its firmware. The client names the two cases that need you. *Needs firmware*: the last update didn't finish — update it again. *Won't start*: its firmware kept restarting, so the board stopped running it. Install a different version — and on a board you flash by ST-Link, switch the cockpit off and on afterwards, or it keeps refusing to start." | Prepped board: red + green, alternating with red alone. Row **WON'T START** — *"its firmware kept restarting"*. Flash the good build by ST-Link, power-cycle the cockpit, LED back to green blinking, row clears. Inset: red + green blinking together at 1 Hz = waiting for firmware (**NEEDS FIRMWARE**). |
| 13 | Screen, then bench (optional, needs prep) | "If an update can't reach a board because its firmware has stopped answering, click Retry, then press the reset button on the board being updated." | Gateway cockpit side: **No response** — *"click Retry, then press the reset button on the Gateway board…"*. Finger on the Gateway's reset button; update runs. |
| 14 | Screen: the docs site | "If the power goes off during an update, follow *Recovering from a power cut during an update* in the Bring-Up guide. Every LED pattern is on the status LED page." | `guides/bring-up/#recovering-from-a-power-cut-during-an-update`, then `firmware/debugging/#in-the-bootloader-red-and-green-together`. |
| 15 | Bench wide | "That's it: flash once, then update from the client — Gateway first, USB side then cockpit side." | End card linking the First Setup & Updates page. |

## Held scenes

Add when the client ships them, each against FirmwarePlan 12 § Rollout / § NODE_ID cases:

- **A PDU from the release** — its row's Update (SkyHawkClient#175), and **Update all** (#176).
- **A PanelGroup board from your build** — *From file…* (#177); Update all skips it as custom firmware.
- **A board with only the bootloader** — the client's blank-board setup (#177), one such board at a time.
- **Moving a board to another node number from the client** (#177).
- Scene 13 on a PDU or PanelGroup board, once SkyHawkClient#175 bench step 8 proves Retry + reset
  through the Gateway.
- Scenes 11 and 12 then switch from ST-Link to the client.
