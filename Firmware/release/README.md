# OpenSkyhawk firmware ${VERSION}

Prebuilt firmware for the OpenSkyhawk boards, built by CI from the release tag `${TAG}`.
Each folder holds one board's files and a short README with the steps to flash it. Every STM32
board has three files: `-full.hex` (the bootloader + the firmware) for a new board and `.elf` (the
firmware alone) for a board that already has the bootloader, both for an ST-Link; and `.bin` (the
firmware alone), which SkyHawkClient loads onto a board that already has the bootloader. The
Gateway's folder also has the `.uf2` for its USB side.

| Folder | Board | Files |
|---|---|---|
${BOARD_TABLE}

**Flash every board from the same release.** Before 1.0, any release may change the CAN wire
format, so boards from different releases may not work together on one bus.

${BOOTLOADER_NOTE}

`CHANGES.md` lists what changed in this release. `manifest.json` lists every file with its path,
size and SHA-256 checksum.

The full guide to flashing, including ST-Link troubleshooting:
https://openskyhawk.github.io/OpenSkyhawk/guides/flashing/

## License

This firmware is licensed under the GNU General Public License v2 (`LICENSE`). The complete
source code for these binaries is the **Source code** archive attached to the same release:
https://github.com/OpenSkyhawk/OpenSkyhawk/releases/tag/${TAG}
