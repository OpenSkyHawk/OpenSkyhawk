#ifdef ARDUINO_ARCH_RP2040

#include "SimGateway.h"
#include <USB.h>

namespace {
    static HardwareSerial* _uart = nullptr;

    static uint8_t  _diagBuf[8];
    static uint8_t  _diagPos = 0;

    static void (*_cbRtt)(uint16_t, uint32_t) = nullptr;
    static void (*_cbHb)(uint8_t, uint16_t)   = nullptr;
    static void (*_cbErr)(uint8_t, uint8_t, uint8_t) = nullptr;

    void dispatchDiag() {
        switch (_diagBuf[1]) {
            case DIAG_RTT: {
                uint16_t seq;    memcpy(&seq,    _diagBuf + 2, 2);
                uint32_t sentMs; memcpy(&sentMs, _diagBuf + 4, 4);
                if (_cbRtt) _cbRtt(seq, sentMs);
                break;
            }
            case DIAG_HB: {
                uint16_t rxc; memcpy(&rxc, _diagBuf + 4, 2);
                if (_cbHb) _cbHb(_diagBuf[2], rxc);
                break;
            }
            case DIAG_ERR:
                if (_cbErr) _cbErr(_diagBuf[2], _diagBuf[3], _diagBuf[4]);
                break;
        }
    }

    void drainUart() {
        while (_uart->available()) {
            uint8_t b = _uart->read();

            // Resync: drop bytes until we see DIAG_MAGIC at position 0
            if (_diagPos == 0 && b != DIAG_MAGIC) continue;

            _diagBuf[_diagPos++] = b;
            if (_diagPos < 8) continue;
            _diagPos = 0;
            dispatchDiag();
        }
    }
}

namespace SimGateway {

void setup(HardwareSerial& panelBridgePort) {
    _uart = &panelBridgePort;

    // Must be set before Joystick.begin() / DcsBios::setup()
    USB.setManufacturer("OpenSkyhawk");
    USB.setProduct("A-4E Skyhawk");
    USB.setVIDPID(0x2E8A, 0x4134);

    _uart->begin(250000);

    Joystick.use16bit();
    Joystick.useManualSend(true);
    Joystick.begin();
    // DcsBios::setup() must be called by the sketch after this
}

void loop() {
    // DcsBios::loop() must be called by the sketch before or after this
    drainUart();
}

void send(uint16_t controlId, uint16_t value) {
    ControlPacket pkt = {controlId, value};
    _uart->write(reinterpret_cast<const uint8_t*>(&pkt), 4);
}

void onDiagRtt(void (*cb)(uint16_t, uint32_t))     { _cbRtt = cb; }
void onDiagHb(void (*cb)(uint8_t, uint16_t))       { _cbHb  = cb; }
void onDiagErr(void (*cb)(uint8_t, uint8_t, uint8_t)) { _cbErr = cb; }

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
