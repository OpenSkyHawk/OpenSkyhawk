// E2E_DCS_Test — input WIRING CHECK (standalone: NO CAN, NO sim, NO OLED, NO stepper)
//
// Pre-step before the full chain. Confirms the four bench inputs are on the right pins and that the
// pull-ups / pot dividers behave, by reading them as RAW GPIO/ADC and printing activity on USART1
// (PA9 = TX, PA10 = RX) @ 115200. Same pins as the E2E_DCS_Test PanelGroup node.
//
//   Encoder REL  DEST_LAT_KNB      A=PA8  B=PB5
//   Encoder DIR  ARC51_FREQ_10MHZ  A=PB3  B=PB4   (JTAG-DP pins — remapped in setup())
//   Pot  AnalogInput    ARC51_VOL  PA2
//   Pot  AnalogMultiPos ARC51_MODE PA3
//
// Flash:  pio run -d Firmware/Examples/E2E_DCS_Test/WiringCheck -t upload
// Watch:  USART1 @ 115200 (the same diag adapter the full bench uses).
//
// Expect at idle: both encoder channels read 1 (external 10k pull-ups, contacts open at a detent).
//   - Turn an encoder  → its A/B toggle through the quadrature sequence and the detent counter moves
//     (one direction +1, the other -1; which way is cosmetic — swap A/B to flip it).
//   - Sweep a pot      → its value tracks ~0..4095 end to end, smooth, no jumps.
// A dead channel never toggles; a mis-wired pot sits at a rail (0 or 4095) or jitters randomly.

#include <Arduino.h>

static const uint8_t ENC1_A = PA8, ENC1_B = PB5;   // REL  DEST_LAT_KNB
static const uint8_t ENC2_A = PB3, ENC2_B = PB4;   // DIR  ARC51_FREQ_10MHZ  (JTAG pins — remapped)
static const uint8_t POT1   = PA2;                 // AnalogInput    ARC51_VOL
static const uint8_t POT2   = PA3;                 // AnalogMultiPos ARC51_MODE

HardwareSerial Diag(PA10, PA9);   // USART1 (RX, TX)

// Gray-code quadrature step for a prev->cur 2-bit state (A<<1|B): +1 / -1 on a valid edge, else 0.
static int8_t qstep(uint8_t prev, uint8_t cur) {
    static const int8_t T[16] = { 0, -1, +1, 0, +1, 0, 0, -1, -1, 0, 0, +1, 0, +1, -1, 0 };
    return T[((prev & 0x3) << 2) | (cur & 0x3)];
}

static uint8_t rd(uint8_t a, uint8_t b) {
    return (uint8_t)((digitalRead(a) << 1) | digitalRead(b));
}

static uint8_t  enc1Prev, enc2Prev;
static int32_t  enc1Cnt = 0, enc2Cnt = 0;   // raw quadrature steps (÷4 = detents)
static uint16_t pot1Last, pot2Last;
static uint32_t lastBeat = 0;

static void reportEnc(const char* tag, uint8_t s, int32_t cnt) {
    Diag.print(tag);
    Diag.print(F(" A=")); Diag.print((s >> 1) & 1);
    Diag.print(F(" B=")); Diag.print(s & 1);
    Diag.print(F("  detent=")); Diag.println(cnt / 4);
}

void setup() {
    // Free PB3/PB4 (JTAG-DP) so the DIR encoder reads. SWD (PA13/PA14) stays live — ST-Link flashes.
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    Diag.begin(115200);

    pinMode(ENC1_A, INPUT); pinMode(ENC1_B, INPUT);     // external 10k pull-ups to 3V3
    pinMode(ENC2_A, INPUT); pinMode(ENC2_B, INPUT);
    pinMode(POT1, INPUT_ANALOG); pinMode(POT2, INPUT_ANALOG);
    analogReadResolution(12);

    enc1Prev = rd(ENC1_A, ENC1_B);
    enc2Prev = rd(ENC2_A, ENC2_B);
    pot1Last = analogRead(POT1);
    pot2Last = analogRead(POT2);

    Diag.println(F("=== E2E input wiring check (NO CAN / NO sim) ==="));
    Diag.print  (F("idle: ENC1 A="));   Diag.print((enc1Prev >> 1) & 1);
    Diag.print  (F(" B="));             Diag.print(enc1Prev & 1);
    Diag.print  (F(" | ENC2 A="));      Diag.print((enc2Prev >> 1) & 1);
    Diag.print  (F(" B="));             Diag.print(enc2Prev & 1);
    Diag.print  (F(" | POT1="));        Diag.print(pot1Last);
    Diag.print  (F(" POT2="));          Diag.println(pot2Last);
    Diag.println(F("turn knobs / sweep pots — each should report below"));
}

void loop() {
    uint8_t e1 = rd(ENC1_A, ENC1_B);
    if (e1 != enc1Prev) { enc1Cnt += qstep(enc1Prev, e1); enc1Prev = e1;
                          reportEnc("[ENC1 REL  DEST_LAT ]", e1, enc1Cnt); }

    uint8_t e2 = rd(ENC2_A, ENC2_B);
    if (e2 != enc2Prev) { enc2Cnt += qstep(enc2Prev, e2); enc2Prev = e2;
                          reportEnc("[ENC2 DIR  ARC51FREQ]", e2, enc2Cnt); }

    uint16_t p1 = analogRead(POT1);
    if (abs((int)p1 - (int)pot1Last) > 20) { pot1Last = p1;
        Diag.print(F("[POT1 AnalogIn  PA2] ")); Diag.println(p1); }

    uint16_t p2 = analogRead(POT2);
    if (abs((int)p2 - (int)pot2Last) > 20) { pot2Last = p2;
        Diag.print(F("[POT2 MultiPos  PA3] ")); Diag.println(p2); }

    // Heartbeat — proves the sketch is alive even when nothing moves, and snapshots the idle reads.
    uint32_t now = millis();
    if (now - lastBeat >= 2000) { lastBeat = now;
        Diag.print(F("[alive] enc1det=")); Diag.print(enc1Cnt / 4);
        Diag.print(F(" enc2det="));        Diag.print(enc2Cnt / 4);
        Diag.print(F(" pot1="));           Diag.print(pot1Last);
        Diag.print(F(" pot2="));           Diag.println(pot2Last);
    }
}
