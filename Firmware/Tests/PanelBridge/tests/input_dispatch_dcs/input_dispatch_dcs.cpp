// PanelBridge — input_dispatch_dcs test
//
// Purpose: verify the four dispatch frames produce the correct DCS-BIOS ASCII command on
// Serial (UART2): ABS (canIdEvt, %u), REL (canIdEvtRel, %+d), DIR (canIdEvtDir, INC/DEC),
// ACTION (canIdEvtAction, TOGGLE). Also feeds one real 0x700+n frame through onCanRx() so the
// CAN range branch itself is exercised, not just the per-slot seam. Also
// verifies out-of-range IDs (0x8700, 0xFFFF) are dropped (nothing sent on Serial).
//
// Hardware: single STM32F103CB board. No second board or CAN bus required.
//   - Serial (UART2 PA2/PA3) → USB-UART adapter → PC terminal at 250000 baud
//   - DiagSerial (USART1 PA9/PA10) at 115200 baud (progress/test log)
//   - STLink for flash
//
// HOW TO USE:
//   1. Flash:   pio run -e test_input_dispatch_dcs -t upload
//   2. Connect PA2 (TX) / PA3 (RX) to USB-UART adapter. Open terminal at 250000 baud.
//   3. Open DiagSerial (USART1 PA9/PA10) at 115200 baud.
//   4. Observe on USB-UART terminal every 1.5 s:
//        ARM_MASTER 1\n
//        ARM_MASTER 0\n
//        ARM_GUN 1\n
//        GEAR_HANDLE 0\n
//   5. For out-of-range IDs (0x8700, 0xFFFF): nothing on USB-UART; DiagSerial shows
//        [BRIDGE] drop ctrl=0x8700
//        [BRIDGE] drop ctrl=0xFFFF
//
// Pass criteria (automatic):
//   Each case asserts itself against PanelBridge::testDcsSendCount(), which advances only when a
//   command is actually emitted. DiagSerial prints [TEST] ok / [TEST] FAIL per case and a tally
//   per cycle. A silently unrouted frame therefore fails on its own instead of depending on
//   someone noticing a missing line on the USB-UART terminal.
//
// Pass criteria (manual, to confirm the wire format the counter cannot see):
//   - Expected ASCII commands appear on USB-UART at correct interval
//   - Out-of-range IDs produce drop log on DiagSerial and no output on USB-UART

#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
#include <STM32Board.h>
#include <A4EC_CmdIds.h>
#include <CANProtocol.h>   // canIdEvtAction, ControlPacketPair — for the RAW frame case

// ABS canIdEvt (%u) / REL canIdEvtRel (%+d) / DIR canIdEvtDir (INC/DEC) /
// ACTION canIdEvtAction (TOGGLE) / RAW = full frame through onCanRx, exercising the range branch
enum Kind { ABS, REL, DIR, ACTION, RAW };

struct Case {
    Kind        kind;
    uint16_t    controlId;
    uint16_t    value;
    const char* label;
    bool        expectOutput;
};

static const Case CASES[] = {
    { ABS, DCSIN_ARM_MASTER,       1,                        "ARM_MASTER ABS 1 -> \"1\"",         true  },
    { ABS, DCSIN_ARM_MASTER,       0,                        "ARM_MASTER ABS 0 -> \"0\"",         true  },
    { ABS, DCSIN_ARM_GUN,          1,                        "ARM_GUN ABS 1 -> \"1\"",            true  },
    { ABS, DCSIN_GEAR_HANDLE,      0,                        "GEAR_HANDLE ABS 0 -> \"0\"",        true  },
    { REL, DCSIN_ASN41_MAGVAR_KNB, (uint16_t)3200,           "MAGVAR_KNB REL +3200 -> \"+3200\"", true  },
    { REL, DCSIN_ASN41_MAGVAR_KNB, (uint16_t)(int16_t)-3200, "MAGVAR_KNB REL -3200 -> \"-3200\"", true  },
    { DIR, DCSIN_ARC51_FREQ_10MHZ, (uint16_t)1,              "FREQ_10MHZ DIR +1 -> \"INC\"",      true  },
    { DIR, DCSIN_ARC51_FREQ_10MHZ, (uint16_t)(int16_t)-1,    "FREQ_10MHZ DIR -1 -> \"DEC\"",      true  },
    { DIR, DCSIN_ARC51_FREQ_10MHZ, (uint16_t)2,              "FREQ_10MHZ DIR +2 (malformed,drop)",false },
    { ACTION, DCSIN_ARM_MASTER,    0,                        "ARM_MASTER ACTION 0 -> \"TOGGLE\"",  true  },
    { ACTION, DCSIN_ARM_MASTER,    1,                        "ARM_MASTER ACTION 1 (malformed,drop)",false },
    { ACTION, 0x0100,              0,                        "ACTION HID-range 0x0100 (drop)",    false },
    { ACTION, 0x8700,              0,                        "ACTION out-of-range 0x8700 (drop)", false },
    { RAW,    DCSIN_ARM_MASTER,    0,                        "ARM_MASTER via real 0x701 frame",   true  },
    { ABS, 0x8700,                 0,                        "0x8700 (drop)",                     false },
    { ABS, 0xFFFF,                 0,                        "0xFFFF (drop)",                     false },
};
static const uint8_t CASE_COUNT = sizeof(CASES) / sizeof(CASES[0]);

static uint8_t  _idx    = 0;
static uint32_t _lastMs = 0;
static uint16_t _passed = 0;
static uint16_t _failed = 0;

// PanelBridge is the bridge itself, so this env builds with NODE_ID=0 and onCanRx() accepts only
// canIdEvtAction(1)..canIdEvtAction(MAX_NODE_ID) — a panel node, never its own ID. Sourcing the
// RAW frame from NODE_ID would put it at 0x700, one below the range, and it would be dropped
// before reaching the branch this case exists to exercise.
static constexpr uint8_t RAW_SRC_NODE = 1;

void setup() {
    STM32Board::diagSerial().begin(115200);
    STM32Board::diagSerial().println(F("=== PanelBridge: input_dispatch_dcs ==="));
    STM32Board::setDebug(true);
    PanelBridge::setup();
    DcsBios::setup();
    STM32Board::diagSerial().println(F("[TEST] input_dispatch_dcs ready"));
    STM32Board::diagSerial().println(F("[TEST] watch USB-UART (250000) for DCS-BIOS commands"));
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();

    uint32_t now = millis();
    if (now - _lastMs < 1500) return;
    _lastMs = now;

    if (_idx >= CASE_COUNT) {
        _idx = 0;
        auto& d = STM32Board::diagSerial();
        d.print(F("[TEST] --- cycle --- passed=")); d.print(_passed);
        d.print(F(" failed=")); d.println(_failed);
        _passed = 0;
        _failed = 0;
    }

    const Case& c = CASES[_idx++];
    auto& d = STM32Board::diagSerial();
    d.print(F("[TEST] dispatch ")); d.println(c.label);
    if (c.expectOutput) d.println(F("[TEST] expect ASCII on USB-UART"));
    else                d.println(F("[TEST] expect drop (no USB-UART output)"));

    const uint32_t sendsBefore = PanelBridge::testDcsSendCount();

    switch (c.kind) {
        case ABS: PanelBridge::testDispatchEvt(c.controlId, c.value); break;
        case REL: PanelBridge::testDispatchRel(c.controlId, c.value); break;
        case DIR: PanelBridge::testDispatchDir(c.controlId, c.value); break;
        case ACTION: PanelBridge::testDispatchAction(c.controlId, c.value); break;
        case RAW: {
            // Enters above the frame-ID range branches, unlike the seams above — the only way to
            // catch a frame range that was never wired into onCanRx().
            ControlPacketPair pair;
            pair.a = { c.controlId, c.value };
            pair.b = { 0x0000, 0 };
            PanelBridge::testFeedCanFrame(canIdEvtAction(RAW_SRC_NODE),
                                          reinterpret_cast<const uint8_t*>(&pair), 8);
            break;
        }
    }

    // Every expectOutput case in CASES routes to DCS, so the counter is the whole verdict here:
    // exactly one send when output is expected, none otherwise. (An expected-HID case would need
    // a different witness — those live in input_dispatch_hid.)
    const uint32_t sent = PanelBridge::testDcsSendCount() - sendsBefore;
    const bool     ok   = c.expectOutput ? (sent == 1) : (sent == 0);
    if (ok) {
        ++_passed;
        d.println(F("[TEST] ok"));
    } else {
        ++_failed;
        d.print(F("[TEST] FAIL ")); d.print(c.label);
        d.print(F(" sent=")); d.print(sent);
        d.print(F(" expected=")); d.println(c.expectOutput ? 1 : 0);
    }
}
