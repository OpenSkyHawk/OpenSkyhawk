/**
 * @file FirmwareVersion.h
 * @brief The firmware release version — one source for every board that reports it.
 *
 * Every board is built from the same release, so there is one version, not one per library.
 * It goes on the wire in two places: SimGateway's calibration HELLO_ACK, and each STM32 node's
 * READY_n payload (#295). release-please rewrites the three numbers below when a firmware release
 * PR merges (generic updater, `x-release-please-*` markers; see release-please-config.json) —
 * never edit them by hand.
 *
 * Shared between STM32 and RP2040. Contains only `#define` constants.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once

#define OPENSKYHAWK_FW_MAJOR 0 // x-release-please-major
#define OPENSKYHAWK_FW_MINOR 3 // x-release-please-minor
#define OPENSKYHAWK_FW_PATCH 0 // x-release-please-patch
