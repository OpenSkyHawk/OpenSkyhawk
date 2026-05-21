#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <CANProtocol.h>

// ── PanelGroup namespace ──────────────────────────────────────────────────────
// Static singleton for CAN sub-node firmware. Reads node_id from PA0 strap
// pin at boot. Dispatches incoming ControlPacket CAN frames to registered
// output objects. Polls registered input objects and sends CAN events.

namespace PanelGroup {
    void    setup();                                    // init STM32Board + CAN filter + heartbeat
    void    loop();                                     // update + CAN drain + input poll + heartbeat
    bool    sendEvent(uint16_t controlId, uint16_t value);  // send INPUT_EVENT over CAN
    uint8_t nodeId();
}

// ── Output objects — DCS state → panel hardware ───────────────────────────────
// Declare at global scope in your sketch. Each object self-registers and is
// dispatched when PanelGroup::loop() receives a matching ControlPacket.

namespace OpenSkyhawk {

class OutputBase {
public:
    static OutputBase* first;
    OutputBase*        next;
    OutputBase();
    virtual void onPacket(uint16_t controlId, uint16_t value) = 0;
};

// Drive a GPIO pin from a DCS-BIOS output bit (mirrors DcsBios::LED).
//   LED warn(A_4E_C_MASTER_CAUTION, 0x4000, PB0);
class LED : public OutputBase {
    uint16_t addr_, mask_;
    uint8_t  pin_;
public:
    LED(uint16_t addr, uint16_t mask, uint8_t pin);
    void onPacket(uint16_t controlId, uint16_t value) override;
};

// Escape hatch: call an arbitrary function with the raw value.
//   IntegerOutput canopy(A_4E_C_CANOPY_POS, [](uint16_t v){ ... });
class IntegerOutput : public OutputBase {
    uint16_t addr_;
    void (*cb_)(uint16_t);
public:
    IntegerOutput(uint16_t addr, void (*cb)(uint16_t));
    void onPacket(uint16_t controlId, uint16_t value) override;
};

} // namespace OpenSkyhawk

// ── Input objects — panel hardware → DCS via CAN ──────────────────────────────
// Declare at global scope. Each object self-registers and is polled by
// PanelGroup::loop(). On state change, calls PanelGroup::sendEvent().
// The matching DCSInput on SimGateway translates the packet to DCS-BIOS.

namespace OpenSkyhawk {

class InputBase {
public:
    static InputBase* first;
    InputBase*        next;
    InputBase();
    virtual void poll() = 0;
};

// 2-position switch → CAN event (0 = up / inactive, 1 = down / active).
//   Switch2Pos ejSafe(A_4E_C_SEAT_EJECT_SAFE, PIN_EJ_SAFE);
class Switch2Pos : public InputBase {
    uint16_t addr_;
    uint8_t  pin_;
    bool     lastStable_, lastRaw_;
    uint32_t debounceMs_;
public:
    Switch2Pos(uint16_t addr, uint8_t pin);
    void poll() override;
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
