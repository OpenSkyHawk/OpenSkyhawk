#ifdef ARDUINO_ARCH_STM32

#include "PduNode.h"
#include "PduConfig.h"
#include "PduMath.h"
#include "TelemSchedule.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

using namespace OpenSkyhawk;
using namespace OpenSkyhawk::Pdu;

namespace {

// Fault sources — globals, as the FaultSource registry requires (static lifetime).
RailMonitor  g12({TelemPage::PDU_RAIL_12V, "12V", PDU_12V_UV_MV, PDU_12V_OV_MV, PDU_12V_OC_MA, true});
RailMonitor  g5 ({TelemPage::PDU_RAIL_5V,  "5V",  PDU_5V_UV_MV,  PDU_5V_OV_MV,  PDU_5V_OC_MA,  false});
BoardThermal gTherm;

CalStore      gStore;
CalRecord     gCal;
CalValues     gVals;
TelemSchedule gSched;

uint16_t gVdda       = VDDA_NOMINAL_MV;
uint32_t gBootMs     = 0;
uint32_t gNextScanUs = 0;
uint32_t gLastSlowMs = 0;
uint32_t gScans      = 0;     // scans in the current second
uint32_t gRateMarkMs = 0;
uint16_t gScanRate   = 0;
bool     gSyncReq    = false;

char    gLine[40];
uint8_t gLineLen = 0;

constexpr uint32_t SLOW_MS = 100;   // NTC + Vrefint cadence

bool armed(uint32_t now) { return now - gBootMs >= PDU_BOOT_BLANK_MS; }

void loadCal() {
    gStore.load(gCal);
    gVals = calValues(gCal);
}

uint8_t statusFlags() {
    uint8_t f = gTherm.flags();
    if (!(gCal.calFlags & CAL_VDDA)) f |= (uint8_t)PduStatusFlag::UNCAL_VDDA;
    if (!(gCal.calFlags & CAL_ZERO)) f |= (uint8_t)PduStatusFlag::UNCAL_ZERO;
    return f;
}

TelemPayload statusPage(bool periodic) {
    const int16_t  t   = gTherm.tempDC();
    const uint16_t p12 = g12.peakMa(), p5 = g5.peakMa();
    TelemPayload p;
    p.page    = (uint8_t)TelemPage::PDU_STATUS;
    p.body[0] = statusFlags();
    p.body[1] = (uint8_t)((uint16_t)t & 0xFF);  p.body[2] = (uint8_t)((uint16_t)t >> 8);
    p.body[3] = (uint8_t)(p12 & 0xFF);          p.body[4] = (uint8_t)(p12 >> 8);
    p.body[5] = (uint8_t)(p5 & 0xFF);           p.body[6] = (uint8_t)(p5 >> 8);
    if (periodic) { g12.resetPeak(); g5.resetPeak(); }
    return p;
}

void sendPage(uint8_t slot, bool periodic, uint32_t now) {
    TelemPayload p = slot == 0 ? g12.buildPage(periodic)
                   : slot == 1 ? g5.buildPage(periodic)
                   :             statusPage(periodic);
    CANProtocol::send(canIdTelem(NODE_ID), reinterpret_cast<const uint8_t*>(&p), sizeof(p));
    gSched.sent(slot, p.body[0], now);
}

void onSyncReq() { gSyncReq = true; }

uint16_t readAdc(uint32_t pin) { return (uint16_t)analogRead(pin); }

void scanRails(uint32_t now) {
    const uint16_t cV12 = readAdc(PDU_PIN_V12), cV5 = readAdc(PDU_PIN_V5);
    const uint16_t cI12 = readAdc(PDU_PIN_I12), cI5 = readAdc(PDU_PIN_I5);
    const bool a = armed(now);
    g12.sample(rail12Mv(countsToMv(cV12, gVdda), gVals.vGain12),
               currentMa(countsToMv(cI12, gVdda), gVals.zero12DmA, gVals.iGain12), railed(cV12), now, a);
    g5.sample(rail5Mv(countsToMv(cV5, gVdda), gVals.vGain5),
              currentMa(countsToMv(cI5, gVdda), gVals.zero5DmA, gVals.iGain5), railed(cV5), now, a);
    gScans++;
}

void scanSlow(uint32_t now) {
    gVdda = vddaMv(readAdc(AVREF), gVals.vrefintMv);
    bool fault = false;
    const int16_t t = ntcTempDC(readAdc(PDU_PIN_NTC), gVals.ntcOffsetDC, fault);
    gTherm.sample(t, fault, armed(now));
}

// ── DiagSerial calibration commands ─────────────────────────────────────────────

Print& out() { return STM32Board::diagSerial(); }

void printCal() {
    auto& d = out();
    d.print(F("[PDU] cal: VDDA="));  d.print((gCal.calFlags & CAL_VDDA) ? F("yes") : F("no"));
    d.print(F(" vrefintMv="));       d.print(gCal.vrefintMv);
    d.print(F(" ZERO="));            d.print((gCal.calFlags & CAL_ZERO) ? F("yes") : F("no"));
    d.print(F(" zero12dmA="));       d.print(gCal.zero12DmA);
    d.print(F(" zero5dmA="));        d.print(gCal.zero5DmA);
    d.print(F(" seq="));             d.print(gCal.seq);
    d.print(F(" slots="));           d.print(gStore.usedSlots()); d.print('/'); d.println(CAL_SLOTS);
}

void saveCal(CalRecord& rec) {
    if (gStore.save(rec)) { loadCal(); out().print(F("[PDU] saved. ")); printCal(); }
    else out().println(F("[PDU] ERROR: flash write failed — calibration not saved"));
}

void cmdCalVdda(const char* arg) {
    const long mv = atol(arg);
    if (mv < 3000 || mv > 3600) { out().println(F("[PDU] usage: CAL VDDA <mV 3000-3600> (DMM on 3V3)")); return; }
    uint32_t sum = 0;
    for (int i = 0; i < 64; i++) sum += readAdc(AVREF);
    const uint32_t raw = sum / 64;
    const uint32_t vref = raw ? (uint32_t)mv * raw / ADC_FULL : 0;
    if (vref < 1100 || vref > 1300) {
        out().print(F("[PDU] REFUSED: derived Vrefint ")); out().print(vref);
        out().println(F(" mV is outside 1100-1300 — check the DMM reading"));
        return;
    }
    CalRecord rec = gCal;
    rec.vrefintMv = (uint16_t)vref;
    rec.calFlags |= CAL_VDDA;
    saveCal(rec);
}

void cmdCalZero() {
    int32_t s12 = 0, s5 = 0;
    for (int i = 0; i < 256; i++) {
        s12 += rawCurrentDmA(countsToMv(readAdc(PDU_PIN_I12), gVdda));
        s5  += rawCurrentDmA(countsToMv(readAdc(PDU_PIN_I5),  gVdda));
    }
    const int32_t z12 = s12 / 256, z5 = s5 / 256;
    out().print(F("[PDU] zero read: 12V ")); out().print(z12 / 10); out().print(F(" mA, 5V "));
    out().print(z5 / 10); out().println(F(" mA (5V includes the PDU's own draw)"));
    if (labs(z12) > PDU_ZERO_GUARD_12V_MA * 10L || labs(z5) > PDU_ZERO_GUARD_5V_MA * 10L) {
        out().println(F("[PDU] REFUSED: current too high for a zero — disconnect the load and retry"));
        return;
    }
    CalRecord rec = gCal;
    rec.zero12DmA = clampI16(z12);
    rec.zero5DmA  = clampI16(z5);
    rec.calFlags |= CAL_ZERO;
    saveCal(rec);
}

void cmdCalClear() {
    // Report "cleared" only when the page really erased — otherwise the old calibration is still
    // what the node is using, and saying otherwise would mislead whoever is calibrating.
    if (!gStore.erase()) {
        out().println(F("[PDU] ERROR: flash erase failed — calibration unchanged"));
        return;
    }
    loadCal();
    out().print(F("[PDU] cleared. "));
    printCal();
}

void cmdRaw() {
    auto& d = out();
    d.print(F("[PDU] vdda=")); d.print(gVdda);
    d.print(F("mV 12V=")); d.print(g12.lastMv()); d.print(F("mV/")); d.print(g12.lastMa());
    d.print(F("mA 5V="));  d.print(g5.lastMv());  d.print(F("mV/")); d.print(g5.lastMa());
    d.print(F("mA temp=")); d.print(gTherm.tempDC()); d.print(F("dC flags12=0x")); d.print(g12.flags(), HEX);
    d.print(F(" flags5=0x")); d.print(g5.flags(), HEX); d.print(F(" status=0x")); d.print(statusFlags(), HEX);
    d.print(F(" scans/s=")); d.println(gScanRate);
}

void runCommand(char* line) {
    for (char* p = line; *p; p++) *p = (char)toupper((unsigned char)*p);
    if      (!strcmp(line, "CAL SHOW"))           printCal();
    else if (!strncmp(line, "CAL VDDA ", 9))      cmdCalVdda(line + 9);
    else if (!strcmp(line, "CAL ZERO"))           cmdCalZero();
    else if (!strcmp(line, "CAL CLEAR"))          cmdCalClear();
    else if (!strcmp(line, "RAW"))                cmdRaw();
    else if (line[0]) out().println(F("[PDU] commands: CAL SHOW | CAL VDDA <mV> | CAL ZERO | CAL CLEAR | RAW"));
}

void pollDiag() {
    auto& d = STM32Board::diagSerial();
    while (d.available()) {
        const char c = (char)d.read();
        if (c == '\r' || c == '\n') {
            gLine[gLineLen] = '\0';
            if (gLineLen) runCommand(gLine);
            gLineLen = 0;
        } else if (gLineLen < sizeof(gLine) - 1) {
            gLine[gLineLen++] = c;
        }
    }
}

} // namespace

namespace PduNode {

void setup() {
    STM32Board::begin();
    loadCal();

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onSyncReq(onSyncReq);
    CANProtocol::start();   // HB_0 accepted automatically; node status arms on the first loop()

    const uint32_t now = millis();
    gBootMs = gLastSlowMs = gRateMarkMs = now;
    gNextScanUs = micros();
    gSched.start(now);
    scanSlow(now);          // VDDA + NTC before the first rail scan

    auto& d = out();
    d.print(F("[PDU] NODE_ID=")); d.println((int)NODE_ID);
    printCal();
    CANProtocol::send(canIdReady(NODE_ID), nullptr, 0);
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();
    CANProtocol::serviceNodeStatus(0);

    const uint32_t us = micros();
    if ((int32_t)(us - gNextScanUs) >= 0) {
        gNextScanUs += PDU_SAMPLE_US;
        if ((int32_t)(us - gNextScanUs) >= (int32_t)(10 * PDU_SAMPLE_US)) gNextScanUs = us + PDU_SAMPLE_US;
        scanRails(millis());
    }

    const uint32_t now = millis();
    if (now - gLastSlowMs >= SLOW_MS) { gLastSlowMs = now; scanSlow(now); }
    if (now - gRateMarkMs >= 1000) { gRateMarkMs = now; gScanRate = (uint16_t)gScans; gScans = 0; }

    if (gSyncReq) {   // host (re)connected: give it every page now; windows keep running
        gSyncReq = false;
        for (uint8_t s = 0; s < TelemSchedule::PAGES; s++) sendPage(s, false, now);
    }
    const uint8_t flags[TelemSchedule::PAGES] = {g12.flags(), g5.flags(), statusFlags()};
    bool periodic = false;
    const int8_t slot = gSched.next(now, flags, periodic);
    if (slot >= 0) sendPage((uint8_t)slot, periodic, now);

    pollDiag();
}

uint16_t scanRateHz() { return gScanRate; }
const RailMonitor&  rail12()  { return g12; }
const RailMonitor&  rail5()   { return g5; }
const BoardThermal& thermal() { return gTherm; }

} // namespace PduNode

#endif // ARDUINO_ARCH_STM32
