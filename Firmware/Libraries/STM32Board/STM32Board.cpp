#ifdef ARDUINO_ARCH_STM32

#include "STM32Board.h"
#include <CANProtocol.h>  // for CanStatus enum values

// ── Private types ─────────────────────────────────────────────────────────────

// Under STM32BOARD_TEST this enum lives in the header (so currentState() can return it);
// keep the two definitions identical.
#ifndef STM32BOARD_TEST
enum class LedState {
    OFF,       ///< Both LEDs off — pre-begin() only
    BOOTING,   ///< Red slow blink (1000 ms) — initialising
    NORMAL,    ///< Green slow blink (1000 ms) — CAN healthy, no data flowing
    CONNECTED, ///< Green solid — CAN healthy and data flowing (DCS exports / CTRL_BCAST)
    CAN_ERROR, ///< Red fast blink (250 ms) — TEC > 0, errors accumulating
    BUS_OFF,   ///< Red solid — CAN controller halted
    WARNING,   ///< Red/green alternating (500 ms) — app-layer degraded state
};
#endif

// ── Private state ─────────────────────────────────────────────────────────────

static LedState          _state      = LedState::OFF;
static bool              _blinkPhase = false;
static uint32_t          _ledLastMs  = 0;
static bool              _debugOn    = false;
static HardwareSerial    _diag(PA10, PA9);  // USART1: RX=PA10, TX=PA9
static CAN_HandleTypeDef _hcan;

// Derived-state inputs — arbitrated by _recompute() into the effective _state.
// Storing inputs (not the final state) lets CONNECTED survive a transient CAN fault and
// re-engage on recovery, and lets WARNING be cleared independently.
static CanStatus _canStatus  = CanStatus::STARTING; ///< Last value from onCanStatus()
static bool      _linkActive = false;               ///< Data flowing (set by setLinkActive)
static uint32_t  _linkLastMs = 0;                   ///< millis() of last setLinkActive(true)
static bool      _warning    = false;               ///< App-layer warning latch
static bool      _begun      = false;               ///< false → OFF (pre-begin)
static bool      _clockFault = false;               ///< SystemClock_Config off HSE / wrong freq (issue #245)

static constexpr uint32_t LINK_DECAY_MS = 500;      ///< CONNECTED → NORMAL after this idle gap

// ── Private helpers ───────────────────────────────────────────────────────────

static uint16_t _blinkPeriodFor(LedState s) {
    switch (s) {
        case LedState::BOOTING:   return 1000;
        case LedState::NORMAL:    return 1000;
        case LedState::CAN_ERROR: return 250;
        case LedState::WARNING:   return 500;
        default:                  return 0;  // solid or off — no timer needed
    }
}

static void _applyLed() {
    switch (_state) {
        case LedState::OFF:
            digitalWrite(STM32Board::PIN_LED_RED,   LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, LOW);
            break;
        case LedState::BOOTING:
        case LedState::CAN_ERROR:
            digitalWrite(STM32Board::PIN_LED_RED,   _blinkPhase ? HIGH : LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, LOW);
            break;
        case LedState::NORMAL:
            digitalWrite(STM32Board::PIN_LED_RED,   LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, _blinkPhase ? HIGH : LOW);
            break;
        case LedState::CONNECTED:
            digitalWrite(STM32Board::PIN_LED_RED,   LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, HIGH);
            break;
        case LedState::BUS_OFF:
            digitalWrite(STM32Board::PIN_LED_RED,   HIGH);
            digitalWrite(STM32Board::PIN_LED_GREEN, LOW);
            break;
        case LedState::WARNING:
            digitalWrite(STM32Board::PIN_LED_RED,   _blinkPhase ? HIGH : LOW);
            digitalWrite(STM32Board::PIN_LED_GREEN, _blinkPhase ? LOW  : HIGH);
            break;
    }
}

// Arbitrate the inputs into the effective LED state by fixed precedence (highest first).
// Sole writer of _state. Resets the blink phase and repaints only on an actual transition,
// preserving the invariant that the phase resets only when the state genuinely changes.
//
//   not begun            → OFF
//   clock fault          → WARNING    (amber alt)      ← root HW fault: outranks the CAN
//                                                        errors it causes, so a bad clock is
//                                                        not misdiagnosed as a bus problem (#245)
//   CAN BUS_OFF          → BUS_OFF    (solid red)      } CAN faults outrank an app-layer
//   CAN TX_ERROR         → CAN_ERROR  (fast red)       } warning — a dead/erroring bus must
//   CAN STARTING         → BOOTING    (slow red)       } not be masked by setWarning().
//   warning              → WARNING    (amber alt)
//   NORMAL + linkActive  → CONNECTED  (solid green)    ← only reachable when CAN is NORMAL
//   NORMAL               → NORMAL     (slow green)
static void _recompute() {
    LedState next;
    if      (!_begun)                            next = LedState::OFF;
    else if (_clockFault)                        next = LedState::WARNING;
    else if (_canStatus == CanStatus::BUS_OFF)   next = LedState::BUS_OFF;
    else if (_canStatus == CanStatus::TX_ERROR)  next = LedState::CAN_ERROR;
    else if (_canStatus == CanStatus::STARTING)  next = LedState::BOOTING;
    else if (_warning)                           next = LedState::WARNING;
    else if (_linkActive)                        next = LedState::CONNECTED;
    else                                         next = LedState::NORMAL;

    if (next != _state) {
        _state      = next;
        _blinkPhase = false;
        _ledLastMs  = millis();
        _applyLed();
    }
}

// HAL weak-symbol override — defines CAN GPIO once for all OpenSkyhawk STM32 boards.
extern "C" void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan_p) {
    if (hcan_p->Instance != CAN1) return;
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin   = GPIO_PIN_12;
    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin  = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

// Strong override of the core's __weak SystemClock_Config. The genericSTM32F103C8/CB
// variant defaults SYSCLK to HSI-PLL 64 MHz → APB1 32 MHz → CAN 444 kbps (spec 500).
// -DHSE_VALUE only tells HAL the crystal frequency; nothing selects HSE. Every
// OpenSkyhawk STM32 node links STM32Board, so selecting HSE here fixes the fleet in
// one place: HSE 8 MHz × 9 = 72 MHz → APB1 36 MHz → CAN 500 kbps, matching begin()'s
// bit timing. If HSE fails to start (dead crystal / cold joint → HSERDY timeout) or the
// configured tree is self-inconsistent, _clockFault latches and begin() raises a visible
// WARNING rather than silently running the wrong rate. NOTE: this does NOT detect a
// wrong-VALUE crystal — HAL_RCC_Get*Freq() derive from the compile-time HSE_VALUE, not a
// measurement, so a 12 MHz part would compute (and pass) as 72 MHz while really running
// 108 MHz. Correct crystal value is a BOM/build guarantee, not runtime-detectable here.
// See issue #245. -DFORCE_CLOCK_FALLBACK exercises the fault path without a dead crystal.
extern "C" void SystemClock_Config(void) {
#ifndef FORCE_CLOCK_FALLBACK
    RCC_OscInitTypeDef osc = {};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;               // 8 MHz × 9 = 72 MHz
    if (HAL_RCC_OscConfig(&osc) == HAL_OK) {
        RCC_ClkInitTypeDef clk = {};
        clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                             RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
        clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
        clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;        // 72 MHz
        clk.APB1CLKDivider = RCC_HCLK_DIV2;          // 36 MHz — CAN, USART2/3
        clk.APB2CLKDivider = RCC_HCLK_DIV1;          // 72 MHz — USART1 diag
        if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) == HAL_OK) {
            // Verify the configured tree is self-consistent (guards a bad edit to the
            // dividers/mult above). These freqs are computed from the RCC config × the
            // compile-time HSE_VALUE, NOT measured — so this cannot catch a wrong-value
            // crystal (see header note); it only confirms the config we just applied.
            if (HAL_RCC_GetSysClockFreq() == 72000000UL &&
                HAL_RCC_GetPCLK1Freq()   == 36000000UL)
                return;                              // 72 MHz locked and verified
        }
    }
#endif
    // HSE failed / forced / wrong frequency → do NOT silently run at the wrong CAN rate.
    // Latch the fault (begin() → WARNING) and limp on internal RC just enough to signal.
    _clockFault = true;
    RCC_OscInitTypeDef h = {};
    h.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    h.HSIState = RCC_HSI_ON;
    h.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    h.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&h);
    RCC_ClkInitTypeDef c = {};
    c.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                  RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    c.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;           // 8 MHz — visibly wrong, safe boot
    c.AHBCLKDivider = RCC_SYSCLK_DIV1;
    c.APB1CLKDivider = RCC_HCLK_DIV1;
    c.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&c, FLASH_LATENCY_0);
}

// ── Public API ────────────────────────────────────────────────────────────────

namespace STM32Board {

void begin() {
    pinMode(PIN_LED_RED,   OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    _begun     = true;
    _canStatus = CanStatus::STARTING;
    _recompute();  // → BOOTING, or WARNING if _clockFault latched in SystemClock_Config (#245)

    _diag.begin(115200);
    analogReadResolution(16);  // STM32duino defaults to 10-bit; 16-bit fills uint16_t directly for PinRef

    // Report the resulting clock + derived CAN bitrate so a wrong-speed board is
    // self-evident on the diag console, not just via the WARNING LED (issue #245).
    if (_debugOn) {
        uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
        _diag.print(_clockFault ? F("CLOCK FAULT: ") : F("CLOCK OK: "));
        _diag.print(F("SYSCLK=")); _diag.print(HAL_RCC_GetSysClockFreq() / 1000000UL); _diag.print(F("MHz "));
        _diag.print(F("PCLK1=")); _diag.print(pclk1 / 1000000UL); _diag.print(F("MHz "));
        _diag.print(F("CAN=")); _diag.print(pclk1 / (4UL * 18UL)); _diag.println(F("bps"));
    }

    // 500 kbps on APB1 @ 36 MHz: prescaler=4, BS1=13TQ, BS2=4TQ → 18TQ total.
    // SJW=4TQ required for Blue Pill clone crystal tolerance (validated Experiment B).
    _hcan.Instance                  = CAN1;
    _hcan.Init.Prescaler            = 4;
    _hcan.Init.Mode                 = CAN_MODE_NORMAL;
    _hcan.Init.SyncJumpWidth        = CAN_SJW_4TQ;
    _hcan.Init.TimeSeg1             = CAN_BS1_13TQ;
    _hcan.Init.TimeSeg2             = CAN_BS2_4TQ;
    _hcan.Init.TimeTriggeredMode    = DISABLE;
    _hcan.Init.AutoBusOff           = ENABLE;   // hardware recovery ~3 ms, no firmware action needed
    _hcan.Init.AutoWakeUp           = DISABLE;
    _hcan.Init.AutoRetransmission   = DISABLE;  // prevents runaway bus-off on unACKed frames
    _hcan.Init.ReceiveFifoLocked    = DISABLE;
    _hcan.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&_hcan);

}

void setDebug(bool on) { _debugOn = on; }
bool isDebug()         { return _debugOn; }
bool clockFault()      { return _clockFault; }

void tick() {
    uint32_t now = millis();

    // Decay the data-flowing link → drop CONNECTED back to NORMAL after a quiet gap.
    if (_linkActive && (now - _linkLastMs) >= LINK_DECAY_MS) {
        _linkActive = false;
        _recompute();
    }

    uint16_t period = _blinkPeriodFor(_state);
    if (period > 0 && (now - _ledLastMs) >= (uint32_t)(period / 2)) {
        _blinkPhase = !_blinkPhase;
        _ledLastMs  = now;
        _applyLed();
    }
}

void onCanStatus(CanStatus status) {
    _canStatus = status;
    _recompute();
}

void setWarning(bool on) {
    _warning = on;
    _recompute();
}

void setLinkActive(bool active) {
    if (active) {
        _linkActive = true;
        _linkLastMs = millis();
    } else {
        _linkActive = false;
    }
    _recompute();
}

void log(const char* msg) {
    if (_debugOn) _diag.println(msg);
}

HardwareSerial& diagSerial()     { return _diag; }
CAN_HandleTypeDef* canHandle()   { return &_hcan; }

// ── Internal die-temperature telemetry ──────────────────────────────────────────
// Free node health from the F103's built-in sensor (ADC ch16) + Vrefint (ch17); no
// external parts. STM32duino's analogRead(ATEMP)/analogRead(AVREF) enable TSVREFE and
// pick the internal channels. Conversion uses F103 datasheet typicals (uncalibrated —
// no factory trim), referencing Vsense to the Vdd derived from Vrefint.
#if defined(ATEMP) && defined(AVREF)
static constexpr uint32_t ADC_FULL     = 1UL << 16;  // full-scale count at analogReadResolution(16)
static constexpr uint32_t VREFINT_MV   = 1200;       // F103 Vrefint typical (no VREFINT_CAL on F103)
static constexpr int32_t  V25_MV       = 1430;       // temp-sensor output at 25 °C, typical
static constexpr int32_t  AVG_SLOPE_10 = 43;         // Avg_Slope 4.3 mV/°C, ×10 for integer math
#endif

uint16_t readVddMv() {
#if defined(ATEMP) && defined(AVREF)
    uint32_t vref = analogRead(AVREF);
    if (vref == 0) return 0;
    return (uint16_t)(VREFINT_MV * ADC_FULL / vref);
#else
    return 0;
#endif
}

int8_t readDieTempC() {
#if defined(ATEMP) && defined(AVREF)
    uint16_t vdd = readVddMv();
    if (vdd == 0) return INT8_MIN;  // internal reference unreadable → unavailable
    uint32_t traw = analogRead(ATEMP);
    int32_t vsense_mv = (int32_t)((uint64_t)traw * vdd / ADC_FULL);
    int32_t tempC = ((V25_MV - vsense_mv) * 10) / AVG_SLOPE_10 + 25;
    if (tempC <= INT8_MIN) tempC = INT8_MIN + 1;  // reserve INT8_MIN as the "unavailable" sentinel
    if (tempC >  INT8_MAX) tempC = INT8_MAX;
    return (int8_t)tempC;
#else
    return INT8_MIN;
#endif
}

void logNodeFaultEdge(const char* tag, NodeFaultCode fault, const char* detail) {
    static NodeFaultCode _prevFault = NodeFaultCode::NONE;
    if (fault != _prevFault && _debugOn) {
        auto& d = diagSerial();
        if (fault != NodeFaultCode::NONE) {
            d.print('['); d.print(tag); d.print(F("] degraded: ")); d.print(detail);
            d.print(F(" (fault ")); d.print((int)(uint8_t)fault); d.println(')');
        } else {
            d.print('['); d.print(tag); d.println(F("] recovered"));
        }
    }
    _prevFault = fault;
}

#ifdef STM32BOARD_TEST
LedState currentState() { return _state; }
#endif

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
