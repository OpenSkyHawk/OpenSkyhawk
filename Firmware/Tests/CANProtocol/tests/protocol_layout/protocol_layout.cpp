// CANProtocol — struct layout and ID function test
//
// All checks are compile-time static_assert. If this builds, all checks pass.
// No runtime assertions or hardware interaction needed.
//
// Expected output on serial:
//   All static_assert passed (compile-time verified)

#include <STM32Board.h>
#include <CANProtocol.h>

static_assert(sizeof(ControlPacket)     == 4, "ControlPacket must be 4 bytes");
static_assert(sizeof(ControlPacketPair) == 8, "ControlPacketPair must be 8 bytes");
static_assert(sizeof(HeartbeatPayload)  == 8, "HeartbeatPayload must be 8 bytes");

static_assert(canIdHb(0)    == 0x100U, "canIdHb(0) reserved for PanelBridge");
static_assert(canIdHb(1)    == 0x101U, "canIdHb(1) must be 0x101");
static_assert(canIdHb(63)   == 0x13FU, "canIdHb(63) must be 0x13F");
static_assert(canIdEvt(1)   == 0x201U, "canIdEvt(1) must be 0x201");
static_assert(canIdEcho(1)  == 0x301U, "canIdEcho(1) must be 0x301");
static_assert(canIdReady(1) == 0x401U, "canIdReady(1) must be 0x401");

static_assert(CAN_ID_CTRL_BCAST == 0x010U, "CTRL_BCAST must be 0x010");
static_assert(CAN_ID_TEST_SEQ   == 0x011U, "TEST_SEQ must be 0x011");
static_assert(CAN_ID_SYNC_REQ   == 0x012U, "SYNC_REQ must be 0x012");

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== protocol_layout ===");
    STM32Board::log("All static_assert passed (compile-time verified).");
}

void loop() {
    STM32Board::tick();
}
