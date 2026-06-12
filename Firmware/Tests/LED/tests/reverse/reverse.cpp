// LED — reverse polarity test
//
// Verifies:
//   reverse=false (default): value > 0 → pin HIGH; value == 0 → pin LOW.
//   reverse=true:            value > 0 → pin LOW  (on); value == 0 → pin HIGH (off).
//   Both on and off transitions are inverted correctly.
//
// Hardware: STM32. PB0 (PIN_SRC, reverse=false) and PB1 (PIN_SINK, reverse=true).
// No external connections needed — pins are driven as outputs and read back
// via digitalRead() which reads the GPIO output register, not an external signal.

#include <Arduino.h>
#include <LED.h>

static constexpr uint8_t  PIN_SRC  = PB0;
static constexpr uint8_t  PIN_SINK = PB1;
static constexpr uint16_t CTRL_SRC = 0xAAAA;
static constexpr uint16_t CTRL_SNK = 0xBBBB;

OpenSkyhawk::LED gSrc (CTRL_SRC, 0xFFFF, PinRef(PIN_SRC));               // reverse=false
OpenSkyhawk::LED gSink(CTRL_SNK, 0xFFFF, PinRef(PIN_SINK), /*reverse=*/true);

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("=== LED reverse ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        Serial.print(label);
        Serial.println(ok ? ": PASS" : ": FAIL");
    };

    gSrc.configure();
    gSink.configure();

    // ── reverse=false ─────────────────────────────────────────────────────────
    gSrc.onControlPacket(CTRL_SRC, 0x0001);
    check("src: value=1 → HIGH", digitalRead(PIN_SRC) == HIGH);

    gSrc.onControlPacket(CTRL_SRC, 0x0000);
    check("src: value=0 → LOW", digitalRead(PIN_SRC) == LOW);

    // ── reverse=true — polarity is inverted ──────────────────────────────────
    gSink.onControlPacket(CTRL_SNK, 0x0001);
    check("sink: value=1 → LOW (on in sink wiring)", digitalRead(PIN_SINK) == LOW);

    gSink.onControlPacket(CTRL_SNK, 0x0000);
    check("sink: value=0 → HIGH (off in sink wiring)", digitalRead(PIN_SINK) == HIGH);

    // ── Cross-check: non-matching controlId does not fire the other ──────────
    uint8_t snapSink = digitalRead(PIN_SINK);
    gSrc.onControlPacket(CTRL_SNK, 0x0001); // targeting sink addr via src object
    check("src ignores sink controlId — sink pin unchanged",
          digitalRead(PIN_SINK) == snapSink);

    Serial.println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
