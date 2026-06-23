/**
 * @file OpenSkyhawk.h
 * @brief Umbrella include for PanelGroup sketch files.
 *
 * Include order matters: PanelGroup.h first (defines InputBase/OutputBase),
 * then concrete classes (their PanelGroup.h re-include is a no-op via #pragma once).
 * A4EC_InputMap.h is NOT included here — it is used only by PanelBridge.cpp.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <STM32Board.h>
#include <PanelGroup.h>
#include <Drivers/StepperMotor/StepperMotor.h>
#include <Outputs/LED/LED.h>
#include <Outputs/NeedleGauge/NeedleGauge.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>
#include <Inputs/Switch3Pos/Switch3Pos.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>
#include <Inputs/AnalogInput/AnalogInput.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>
#include <A4EC_CmdIds.h>
#include <A4EC_OutputIds.h>
