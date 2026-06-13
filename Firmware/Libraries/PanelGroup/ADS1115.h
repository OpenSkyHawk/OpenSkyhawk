/**
 * @file ADS1115.h
 * @brief Thin wrapper over Adafruit_ADS1115 that provides a forward-declarable class name.
 *
 * @details PinRef.h forward-declares `class ADS1115`. A typedef alias cannot be forward-declared
 * with `class`, so this file defines a concrete wrapper class that inherits every constructor and
 * method from Adafruit_ADS1115 without adding any logic.
 *
 * Adafruit_ADS1115 takes address and bus via begin(), not the constructor. Pattern:
 *   ADS1115 adc;
 *   PanelGroup::registerADC(adc, 0x48, Wire);
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Adafruit_ADS1X15.h>

/**
 * @brief ADS1115 ADC. Inherits Adafruit_ADS1115 without modification.
 *
 * @details Exists so `class ADS1115;` can be forward-declared in PinRef.h.
 * All constructors, methods, and behaviour are identical to Adafruit_ADS1115.
 */
class ADS1115 : public Adafruit_ADS1115 {
public:
    using Adafruit_ADS1115::Adafruit_ADS1115;
};

#endif // ARDUINO_ARCH_STM32
