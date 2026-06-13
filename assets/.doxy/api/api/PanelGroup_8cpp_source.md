

# File PanelGroup.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PanelGroup.cpp**](PanelGroup_8cpp.md)

[Go to the documentation of this file](PanelGroup_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "PanelGroup.h"
#include <STM32Board.h>

// ── Internal state ────────────────────────────────────────────────────────────

namespace {

static constexpr uint8_t MAX_EXPANDERS = 8;
static constexpr uint8_t MAX_ADCS      = 8;
static constexpr uint8_t MAX_INT_PINS  = 8;  // max unique STM32 interrupt pins
static constexpr uint8_t NO_INT_PIN    = 0xFF; // polling-fallback sentinel

struct ExpanderEntry {
    MCP23017* chip;
    uint8_t   intaPin;
    uint8_t   intbPin;
    bool      mirrored;   // intaPin == intbPin at registration; MIRROR mode
    bool      openDrain;  // shares interrupt line; IOCON.ODR = 1
    uint8_t   portAcache; // last-known GPA0–7 state
    uint8_t   portBcache; // last-known GPB0–7 state
};

struct ADCEntry {
    ADS1115*  adc;
    uint8_t   addr;
    TwoWire*  wire;
};

static ExpanderEntry _expanders[MAX_EXPANDERS];
static uint8_t       _expanderCount = 0;

static ADCEntry _adcs[MAX_ADCS];
static uint8_t  _adcCount = 0;

// Unique STM32 interrupt pins and their flags
static uint8_t          _intPins[MAX_INT_PINS];
static uint8_t          _intPinCount = 0;
static volatile bool    _intFlags[MAX_INT_PINS];

// Timers
static uint32_t _lastHeartbeatMs  = 0;
static uint32_t _lastFallbackMs   = 0;
static uint16_t _rxCount          = 0; // diagnostic only; wraps at 65535 (~131 s at full bus load)

// ── ISR functions — one static function per interrupt-pin slot ───────────────
// No dynamic ISR registration; each slot has a dedicated handler.
// _intFlags is volatile; ISR writes are single-byte — atomic on ARM Cortex-M3.

static void isr0() { _intFlags[0] = true; }
static void isr1() { _intFlags[1] = true; }
static void isr2() { _intFlags[2] = true; }
static void isr3() { _intFlags[3] = true; }
static void isr4() { _intFlags[4] = true; }
static void isr5() { _intFlags[5] = true; }
static void isr6() { _intFlags[6] = true; }
static void isr7() { _intFlags[7] = true; }

static constexpr void (*_isrTable[MAX_INT_PINS])() = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
};

// Returns the slot index for the given STM32 pin, adding it if not yet seen.
// Returns 0xFF if the slot table is full.
static uint8_t findOrAddIntPin(uint8_t pin) {
    for (uint8_t i = 0; i < _intPinCount; i++)
        if (_intPins[i] == pin) return i;
    if (_intPinCount < MAX_INT_PINS) {
        _intPins[_intPinCount] = pin;
        _intFlags[_intPinCount] = false;
        return _intPinCount++;
    }
    return 0xFF;
}

// ── CAN callbacks ─────────────────────────────────────────────────────────────

static void onCanReceive(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != CAN_ID_CTRL_BCAST) return;
    if (len != sizeof(ControlPacketPair)) return; // malformed — discard
    _rxCount++;
    ControlPacketPair pair;
    memcpy(&pair, data, sizeof(pair));
    auto dispatch = [](const ControlPacket& pkt) {
        if (pkt.controlId == 0x0000) return;
        for (auto* o = OpenSkyhawk::OutputBase::head(); o; o = o->next())
            o->onControlPacket(pkt.controlId, pkt.value);
    };
    dispatch(pair.a);
    dispatch(pair.b);
}

static void onSyncReq() {
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next())
        p->forceReport();
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
}

} // anonymous namespace

// ── OpenSkyhawk::InputBase ────────────────────────────────────────────────────

OpenSkyhawk::InputBase*  OpenSkyhawk::InputBase::_head  = nullptr;
OpenSkyhawk::OutputBase* OpenSkyhawk::OutputBase::_head = nullptr;

OpenSkyhawk::InputBase::InputBase() : _next(_head) { _head = this; }
OpenSkyhawk::OutputBase::OutputBase() : _next(_head) { _head = this; }

OpenSkyhawk::InputBase*  OpenSkyhawk::InputBase::head()        { return _head; }
OpenSkyhawk::InputBase*  OpenSkyhawk::InputBase::next()  const { return _next; }
OpenSkyhawk::OutputBase* OpenSkyhawk::OutputBase::head()       { return _head; }
OpenSkyhawk::OutputBase* OpenSkyhawk::OutputBase::next() const { return _next; }

// ── PanelGroup::registerADC / registerExpander ────────────────────────────────

namespace PanelGroup {

void registerADC(ADS1115& adc, uint8_t addr, TwoWire& wire) {
    if (_adcCount >= MAX_ADCS) return;
    // Deduplicate: skip if already registered
    for (uint8_t i = 0; i < _adcCount; i++)
        if (_adcs[i].adc == &adc) return;
    auto& e = _adcs[_adcCount++];
    e.adc  = &adc;
    e.addr = addr;
    e.wire = &wire;
}

void registerExpander(MCP23017& chip, uint8_t intaPin, uint8_t intbPin) {
    if (_expanderCount >= MAX_EXPANDERS) return;
    auto& e    = _expanders[_expanderCount++];
    e.chip      = &chip;
    e.intaPin   = intaPin;
    e.intbPin   = intbPin;
    e.mirrored  = (intaPin == intbPin);
    e.openDrain = false; // resolved in setup()
    e.portAcache = 0xFF;
    e.portBcache = 0xFF;
}

void registerExpander(MCP23017& chip) {
    registerExpander(chip, NO_INT_PIN, NO_INT_PIN);
}

// ── PanelGroup::setup ─────────────────────────────────────────────────────────

void setup() {
    // Step 1 — STM32Board init
    STM32Board::begin();
    CANProtocol::onStatusChange(STM32Board::onCanStatus);

    // Step 2a — ADC begin (address and bus were captured at registerADC time)
    for (uint8_t i = 0; i < _adcCount; i++)
        _adcs[i].adc->begin(_adcs[i].addr, _adcs[i].wire);

    // Step 2b — determine open-drain: any two expanders sharing an interrupt pin
    for (uint8_t i = 0; i < _expanderCount; i++) {
        if (_expanders[i].intaPin == NO_INT_PIN) continue;
        for (uint8_t j = 0; j < _expanderCount; j++) {
            if (i == j) continue;
            if (_expanders[j].intaPin == NO_INT_PIN) continue;
            bool shared = (_expanders[i].intaPin == _expanders[j].intaPin)
                       || (_expanders[i].intaPin == _expanders[j].intbPin)
                       || (_expanders[i].intbPin != NO_INT_PIN &&
                           (_expanders[i].intbPin == _expanders[j].intaPin
                         || _expanders[i].intbPin == _expanders[j].intbPin));
            if (shared) { _expanders[i].openDrain = true; break; }
        }
    }

    // Step 2c — chip init and IOCON configuration
    for (uint8_t i = 0; i < _expanderCount; i++) {
        auto& e = _expanders[i];
        e.chip->init();

        if (e.mirrored) {
            e.chip->interruptMode(MCP23017InterruptMode::Or);
        }
        if (e.openDrain) {
            uint8_t iocon = e.chip->readRegister(MCP23017Register::IOCON);
            e.chip->writeRegister(MCP23017Register::IOCON, iocon | 0x04u); // ODR bit
        }
    }

    // Step 3 — configure each pin via its owning input/output object
    for (auto* p = OpenSkyhawk::InputBase::head();  p; p = p->next()) p->configure();
    for (auto* p = OpenSkyhawk::OutputBase::head(); p; p = p->next()) p->configure();

    // Step 2d — enable interrupt-on-change, read baseline, attach STM32 ISRs
    for (uint8_t i = 0; i < _expanderCount; i++) {
        auto& e = _expanders[i];

        // Enable interrupt-on-change only on input pins, excluding GPA7/GPB7.
        // GPA7/GPB7 must be outputs per Microchip silicon bug (Rev D erratum):
        // asserting GPINTEN on them while in input mode corrupts SDA mid-transfer.
        // IODIR bit=1 means input; mask to 0x7F to exclude bit 7 on both ports.
        uint8_t gpintenA = e.chip->readRegister(MCP23017Register::IODIR_A) & 0x7Fu;
        uint8_t gpintenB = e.chip->readRegister(MCP23017Register::IODIR_B) & 0x7Fu;
        e.chip->writeRegister(MCP23017Register::GPINTEN_A, gpintenA);
        e.chip->writeRegister(MCP23017Register::GPINTEN_B, gpintenB);

        // Baseline read — captures initial state after configure() sets IODIR
        e.portAcache = e.chip->readPort(MCP23017Port::A);
        e.portBcache = e.chip->readPort(MCP23017Port::B);

        if (e.intaPin == NO_INT_PIN) continue; // polling-fallback chip

        uint8_t slotA = findOrAddIntPin(e.intaPin);
        if (slotA == 0xFF) {
            STM32Board::log("[PanelGroup] MAX_INT_PINS exceeded — chip falls back to polling");
            continue;
        }
        // Attach INTA ISR if this is the first chip claiming this pin
        if (_intPins[slotA] == e.intaPin) {
            bool firstClaim = true;
            for (uint8_t k = 0; k < i; k++) {
                if (_expanders[k].intaPin == e.intaPin || _expanders[k].intbPin == e.intaPin) {
                    firstClaim = false; break;
                }
            }
            if (firstClaim) {
                pinMode(e.intaPin, INPUT_PULLUP);
                attachInterrupt(digitalPinToInterrupt(e.intaPin), _isrTable[slotA], FALLING);
            }
        }

        if (!e.mirrored && e.intbPin != NO_INT_PIN) {
            uint8_t slotB = findOrAddIntPin(e.intbPin);
            if (slotB != 0xFF) {
                bool firstClaim = true;
                for (uint8_t k = 0; k < i; k++) {
                    if (_expanders[k].intaPin == e.intbPin || _expanders[k].intbPin == e.intbPin) {
                        firstClaim = false; break;
                    }
                }
                if (firstClaim) {
                    pinMode(e.intbPin, INPUT_PULLUP);
                    attachInterrupt(digitalPinToInterrupt(e.intbPin), _isrTable[slotB], FALLING);
                }
            }
        }
    }

    // Step 4/5 — CAN callbacks and start
    CANProtocol::onReceive(onCanReceive);
    CANProtocol::onSyncReq(onSyncReq);
    CANProtocol::start();

    // Step 6 — boot EVT burst
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) p->forceReport();

    // Step 7 — flush trailing batch
    CANProtocol::flushBatched(canIdEvt(NODE_ID));

    // Step 8 — READY frame + arm heartbeat timer
    CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
    _lastHeartbeatMs = millis();
}

// ── PanelGroup::loop ──────────────────────────────────────────────────────────

void loop() {
    uint32_t now = millis();

    // 1. Interrupt dispatch: check flags, read INTCAP, update caches
    for (uint8_t slot = 0; slot < _intPinCount; slot++) {
        if (!_intFlags[slot]) continue;
        _intFlags[slot] = false;
        uint8_t pin = _intPins[slot];

        // All chips on this line — must visit all to clear open-drain assertion
        for (uint8_t j = 0; j < _expanderCount; j++) {
            auto& e = _expanders[j];
            if (e.intaPin != pin && e.intbPin != pin) continue;

            uint8_t intfA = 0, intfB = 0;
            e.chip->interruptedBy(intfA, intfB);
            if (intfA == 0 && intfB == 0) continue;

            uint8_t capA = 0, capB = 0;
            e.chip->clearInterrupts(capA, capB);
            // INTCAP (capA/capB) holds the pin state at interrupt time, not now.
            // A fast bounce can clear before loop() runs, leaving the cache stale.
            // Read live GPIO to guarantee cache matches current pin state.
            if (intfA) e.portAcache = e.chip->readPort(MCP23017Port::A);
            if (intfB) e.portBcache = e.chip->readPort(MCP23017Port::B);
        }
    }

    // 2. Polling fallback (~20 ms) for chips with no interrupt pin
    if (now - _lastFallbackMs >= 20) {
        _lastFallbackMs = now;
        for (uint8_t i = 0; i < _expanderCount; i++) {
            auto& e = _expanders[i];
            if (e.intaPin != NO_INT_PIN) continue;
            e.portAcache = e.chip->readPort(MCP23017Port::A);
            e.portBcache = e.chip->readPort(MCP23017Port::B);
        }
    }

    // 3. Poll all inputs
    for (auto* p = OpenSkyhawk::InputBase::head(); p; p = p->next()) p->poll();

    // 4. CAN drain: CTRL_BCAST → onControlPacket; SYNC_REQ → onSyncReq; TEST_SEQ → echo
    CANProtocol::drain();

    // 5. Update all outputs (steppers, PWM)
    for (auto* p = OpenSkyhawk::OutputBase::head(); p; p = p->next()) p->update();

    // 6. Heartbeat every 500 ms
    if (now - _lastHeartbeatMs >= 500) {
        _lastHeartbeatMs = now;
        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(NODE_ID, _rxCount);
        CANProtocol::send(canIdHb(NODE_ID),
                          reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
    }
}

// ── MCP cache bridge ──────────────────────────────────────────────────────────

bool readCachedPin(const MCP23017& chip, uint8_t port, uint8_t bit) {
    for (uint8_t i = 0; i < _expanderCount; i++) {
        if (_expanders[i].chip == &chip) {
            uint8_t cache = (port == 0) ? _expanders[i].portAcache
                                        : _expanders[i].portBcache;
            return (cache >> bit) & 1u;
        }
    }
    return false;
}

void writeCachedPin(MCP23017& chip, uint8_t port, uint8_t bit, bool value) {
    for (uint8_t i = 0; i < _expanderCount; i++) {
        if (_expanders[i].chip != &chip) continue;
        uint8_t& cache = (port == 0) ? _expanders[i].portAcache
                                     : _expanders[i].portBcache;
        if (value) cache |=  (1u << bit);
        else       cache &= ~(1u << bit);
        chip.digitalWrite(port * 8 + bit, value ? HIGH : LOW);
        return;
    }
}

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
```


