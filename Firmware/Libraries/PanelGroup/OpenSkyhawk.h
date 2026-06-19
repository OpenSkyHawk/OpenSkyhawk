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
#include <Outputs/LED/LED.h>
#include <Inputs/Switch2Pos/Switch2Pos.h>
#include <A4EC_CmdIds.h>
#include <A4EC_OutputIds.h>
