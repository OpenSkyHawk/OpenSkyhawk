/**
 * @file Uploader.h
 * @brief The Gateway's firmware uploader (#366) — stages an image the client streams over USB, then
 *        flashes a board's bootloader over CAN through PanelBridge, on its own.
 *
 * Library-internal: SimGateway.cpp feeds it the client's UPL_* frames, every line PanelBridge sends
 * the host, and a tick from loop(); the uploader answers through the callbacks in Uploader::Io.
 * Portable C++ — no Arduino, no globals of the caller's — so it can be exercised off-target.
 *
 * Contract: FirmwarePlan/07-simgateway-api.md § Uploader, UploadProtocol.h, BootProtocol.h.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once

#include <stdint.h>

namespace Uploader {

/// How the uploader reaches the client and PanelBridge.
struct Io {
    void (*send)(uint8_t type, uint8_t seq, const uint8_t* payload, uint16_t len);   ///< a frame to the client
    void (*ack)(uint8_t type, uint8_t seq);
    void (*nack)(uint8_t type, uint8_t seq, uint8_t reason, uint8_t detail);
    void (*writeUart)(const uint8_t* p, uint16_t n);                                  ///< bytes to PanelBridge
};

/// Install the callbacks. Call once from SimGateway::begin().
void begin(const Io& io);

/// True for the CAL_T_UPL_* request types the uploader answers.
bool handles(uint8_t type);

/// A verified client frame of one of those types.
void onFrame(uint8_t type, uint8_t seq, const uint8_t* payload, uint16_t len, uint32_t now);

/// Any verified client frame (KEEPALIVE included): the client is still there.
void onClientFrame(uint32_t now);

/// A complete line from PanelBridge (without the '\n'). Returns true when the line must not be
/// relayed to the client — `_BOOT_RSP` while the uploader itself is flashing.
bool onBridgeLine(const char* line, uint16_t len, uint32_t now);

/// Advance the flash. Call every loop().
void tick(uint32_t now);

/// An upload session is open or a flash runs — the calibration session must answer BUSY.
bool busy();

/// A flash runs: the uploader writes the UART, so the client's export bytes must be dropped.
bool ownsUart();

/// Flashing the bridge (#367): its bootloader answers on the UART in its own framing, so every byte
/// PanelBridge sends goes to onUartByte() instead of the line / HID parser — until the bridge reboots.
bool ownsUartRx();
void onUartByte(uint8_t b, uint32_t now);

}  // namespace Uploader
