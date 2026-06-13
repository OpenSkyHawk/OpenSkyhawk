## 1. Firmware Plan completeness

**Files found:** `Firmware/ScratchPad/FirmwarePlan/README.md` is the master index. It says the directory is the authoritative firmware spec split by contract boundary, describes the PC/RP2040 SimGateway/STM32 PanelBridge/CAN PanelGroup topology, and maps each document to a responsibility (`Firmware/ScratchPad/FirmwarePlan/README.md:1-40`). It also says `Firmware/ScratchPad/REQUIREMENTS.md` is superseded by FirmwarePlan (`Firmware/ScratchPad/FirmwarePlan/README.md:58-66`).

| File | Coverage |
| --- | --- |
| `00-decisions.md` | Architecture decisions: SimGateway byte relay, PanelBridge owns DCS-BIOS, `controlId` routing, `NODE_ID` build flags, batching, and reserved heartbeat behavior (`Firmware/ScratchPad/FirmwarePlan/00-decisions.md:12-40`, `Firmware/ScratchPad/FirmwarePlan/00-decisions.md:157-205`). |
| `01-system-overview.md` | System goal, topology, library roles, and data flow. Listed by README as the system overview (`Firmware/ScratchPad/FirmwarePlan/README.md:22-25`). |
| `02-can-protocol.md` | CAN frame IDs, payloads, `NODE_ID`, `ControlPacket`, `ControlPacketPair`, filters, queueing, and bus config (`Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:1-4`, `Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:8-28`, `Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:32-57`, `Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:75-86`). |
| `03-uart-usb-hid-protocol.md` | PanelBridge to SimGateway UART protocol, HID frame format, resync behavior, USB identity, and TinyUSB HID backend (`Firmware/ScratchPad/FirmwarePlan/03-uart-usb-hid-protocol.md:10-91`). |
| `04-dcs-bios-integration.md` | DCS/HID `controlId` ranges, `DCSIN_*` IDs, generated headers, input map, value encoding, and generator rules (`Firmware/ScratchPad/FirmwarePlan/04-dcs-bios-integration.md:10-140`). |
| `05-panelgroup-api.md` | `PinRef`, MCP23017/ADS1115 support, PanelGroup routing, implemented `Switch2Pos`/`LED`, and planned input/output classes (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:12-149`, `Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:151-451`). |
| `06-panelbridge-api.md` | Node tracking, DCS-BIOS export listener, DCS/HID dispatch, `SYNC_REQ`, and test sequence behavior (`Firmware/ScratchPad/FirmwarePlan/06-panelbridge-api.md:10-183`). |
| `07-simgateway-api.md` | RP2040 SimGateway role, HID axes/buttons/hats, allocation, platform compatibility, HID dispatch, relay contract, UART pins, and testing contract (`Firmware/ScratchPad/FirmwarePlan/07-simgateway-api.md:10-225`). |
| `08-hardware-firmware-contracts.md` | STM32 reserved pins, UART pins, DRV8833 sleep, MCP constraints, ADS1115 assumptions, CAN transceiver, diagnostic serial, and status LEDs (`Firmware/ScratchPad/FirmwarePlan/08-hardware-firmware-contracts.md:10-156`). |
| `09-startup-resync-diagnostics.md` | PanelGroup/PanelBridge/SimGateway boot flow, sync behavior, timing races, and diagnostic frames (`Firmware/ScratchPad/FirmwarePlan/09-startup-resync-diagnostics.md:10-146`). |
| `10-implementation-plan.md` | Implementation phases 0-6 and completion status (`Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:8-200`). |
| `11-open-issues.md` | No active open issues, plus resolved decisions and deferred PS3/PS4 compatibility note (`Firmware/ScratchPad/FirmwarePlan/11-open-issues.md:10-50`). |

**Master index:** Yes. The README is the index and states the FirmwarePlan is authoritative for current firmware work (`Firmware/ScratchPad/FirmwarePlan/README.md:1-5`, `Firmware/ScratchPad/FirmwarePlan/README.md:58-66`).

**Complete sections:** `10-implementation-plan.md` marks Phase 0, Phase 1, Phase 2, and Phase 3 as done (`Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:8-162`). `11-open-issues.md` says there are no active open issues (`Firmware/ScratchPad/FirmwarePlan/11-open-issues.md:10`).

**Deferred/TBD:** Phase 4 through Phase 6 remain backlog/future work: more control types, hardware/output implementations, tests, docs, and first full panel integration (`Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:165-200`). `07-simgateway-api.md` still treats PlayStation 3/4 HID compatibility as optional future work (`Firmware/ScratchPad/FirmwarePlan/07-simgateway-api.md:99-142`), and `11-open-issues.md` keeps that as explicitly deferred (`Firmware/ScratchPad/FirmwarePlan/11-open-issues.md:39-50`).

**Contradictions with TechSpec:** Several FirmwarePlan statements are stale relative to `Firmware/ScratchPad/TechSpec/` and source:

- `Switch2Pos` is still described as needing a `PinRef` update in the plan (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:151-158`) and as deferred in Phase 3 (`Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:129-132`), but the TechSpec marks it done and the implementation now uses `PinRef` (`Firmware/ScratchPad/TechSpec/PanelGroup/Inputs/Switch2Pos.md:3`, `Firmware/Libraries/PanelGroup/Switch2Pos.cpp:14-64`).
- `IntegerOutput` is planned, not implemented. FirmwarePlan and `PanelGroup/README.md` still describe it as existing, but its TechSpec says not started and no OpenSkyhawk implementation class was found (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:374-381`, `Firmware/Libraries/PanelGroup/README.md:37-60`, `Firmware/ScratchPad/TechSpec/PanelGroup/Outputs/IntegerOutput.md:3`).
- The plan says the generator has 149 inputs and one unsupported paired boolean gap (`Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:65-66`), while generated metadata currently says 150 controls and no unsupported controls (`Firmware/Libraries/A4EC/A4EC_CmdIds.h:1-6`, `Firmware/Libraries/A4EC/GENERATOR_GAPS.md:7-9`).

**Owner clarification:** FirmwarePlan is internal planning material, not public docs, but it should be updated wherever it conflicts with TechSpec/source because it will be used as input for better future docs.

## 2. CAN message format

**Formal spec:** Yes. `Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md` owns the CAN contract (`Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:1-4`). The implementation mirrors it in `Firmware/Libraries/CANProtocol/CANProtocol.h`.

**Frame ID structure:** Fixed frames are `CTRL_BCAST = 0x010`, `TEST_SEQ = 0x011`, and `SYNC_REQ = 0x012`. Per-node frames are computed ranges for `HB`, `EVT`, `ECHO`, and `READY` (`Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:61-86`, `Firmware/Libraries/CANProtocol/CANProtocol.h:61-79`).

**Payload layout:** `ControlPacket` is 4 bytes: little-endian `uint16_t controlId` plus little-endian `uint16_t value`. `ControlPacketPair` is 8 bytes and batches two `ControlPacket` entries. Heartbeat payload is 8 bytes: `nodeId`, flags, uptime seconds, accepted RX count, and shifted CAN error-status register (`esr`) (`Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:32-57`, `Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:93-155`, `Firmware/Libraries/CANProtocol/CANProtocol.h:23-51`).

**Address envelope:** DCS-BIOS compact input IDs are `0x8000-0x86FF` in the implementation (`Firmware/Libraries/CANProtocol/CANProtocol.h:81-92`). HID now reserves the broader `0x0010-0x00FF` envelope: axes `0x0010-0x001F`, hats `0x0020-0x002F`, buttons `0x0030-0x00AF`, and reserved HID expansion `0x00B0-0x00FF` (`Firmware/Libraries/HIDControls/HIDControls.h:8-45`). Double-check result: no current `CTRL_*` constants occupy `0x00B0-0x00FF`, and there is no conflict with DCS (`>=0x8000`), null sentinel (`0x0000`), or `CTRL_ID_TEST_SEQ` (`0xFFFF`). The broader range is safe as a HID routing envelope, but docs should avoid implying all remaining slots are USB buttons because the current HID descriptor exposes 128 buttons.

**CAN ID namespace:** System/test CAN arbitration IDs do not conflict with HID/DCS `controlId` values because they are separate namespaces. `CAN_ID_TEST_SEQ = 0x011` numerically overlaps `CTRL_PITCH = 0x0011`, but the former is the CAN frame ID and the latter lives inside the `ControlPacket` payload; `CANProtocol::drain()` handles TEST_SEQ by `frame.canId == CAN_ID_TEST_SEQ`. `CTRL_ID_TEST_SEQ = 0xFFFF` remains only as a legacy payload sentinel.

**DCS vs HID routing:** PanelBridge routes by payload `controlId`, not by CAN frame ID alone. If `controlId` is in `0x8000-0x86FF`, it dispatches to DCS-BIOS input handling; if it is below `0x8000`, it emits a HID frame to SimGateway; other values are dropped (`Firmware/Libraries/PanelBridge/PanelBridge.cpp:167-179`). SimGateway only acts on HID handlers registered in its axis/button/hat linked lists (`Firmware/Libraries/SimGateway/SimGateway.cpp:248-275`).

**CAN frame ID routing:** CAN frame ID chooses the transport class and node source/destination: PanelGroups listen for `CTRL_BCAST`; PanelBridge receives `HB`, `EVT`, `ECHO`, and `READY`; `SYNC_REQ` is broadcast (`Firmware/Libraries/PanelGroup/PanelGroup.cpp:79-99`, `Firmware/Libraries/PanelBridge/PanelBridge.cpp:203-245`). Logical control routing is still done on `controlId`.

**NODE_ID scheme:** `NODE_ID` is a compile-time build flag. `0` is reserved for PanelBridge, `1-63` are PanelGroup nodes, and SimGateway has no CAN `NODE_ID` (`Firmware/ScratchPad/FirmwarePlan/02-can-protocol.md:8-28`, `Firmware/Libraries/STM32Board/STM32Board.h:28-29`). Current assignments found:

- E2E and template PanelBridge projects use `NODE_ID=0`.
- E2E and template PanelGroup projects use `NODE_ID=1`.
- Prototype CAN master/subnode projects use `NODE_ID=0` and `NODE_ID=1`.
- The production `Center_Armament` docs still say its CAN node ID is TBD (`docs/claude/controllers/Center_Armament.md:13`), and its current sketch is a stub that does not include `STM32Board` or require `NODE_ID` (`Firmware/Center_Armament/src/main.cpp:1-12`).

## 3. Firmware implementation vs plan - drift check

**Projects found outside ScratchPad:**

| Project | Role | Current state |
| --- | --- | --- |
| `Firmware/Center_Armament` | Intended production STM32 CAN controller for the Armament group. It should host Armament plus AWRS/Misc breakouts (`Firmware/Center_Armament/README.md:1-9`). | Compiles, but source is stub only with TODO setup/loop (`Firmware/Center_Armament/src/main.cpp:1-12`). |
| `Firmware/Prototype/E2E_DCS_Test/PanelBridge` | Current E2E PanelBridge sketch: DCS-BIOS on USB serial, CAN bridge, UART to SimGateway (`Firmware/Prototype/E2E_DCS_Test/PanelBridge/src/main.cpp:1-10`). | Compiles. Implements current three-tier bridge skeleton (`Firmware/Prototype/E2E_DCS_Test/PanelBridge/src/main.cpp:12-35`). |
| `Firmware/Prototype/E2E_DCS_Test/PanelGroup` | Current E2E PanelGroup node: button input to `DCSIN_MASTER_TEST`, LED output from `GEAR_LIGHT` (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:1-14`). | Compiles. Minimal E2E node, not full panel integration (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:16-23`). |
| `Firmware/Prototype/E2E_DCS_Test/SimGateway` | Current E2E RP2040 SimGateway sketch for DCS-BIOS byte relay (`Firmware/Prototype/E2E_DCS_Test/SimGateway/src/main.cpp:1-6`). | Compiles. DCS-only test sketch; no HID axes/buttons declared in the sketch (`Firmware/Prototype/E2E_DCS_Test/SimGateway/src/main.cpp:14-23`). |
| `Firmware/Templates/PanelBridge` | Reusable PanelBridge template. | Compiles and follows the three-tier architecture. |
| `Firmware/Templates/PanelGroup` | Reusable PanelGroup template. | Compiles and follows the three-tier architecture. |
| `Firmware/Templates/SimGateway` | Reusable SimGateway template. | Compiles and follows the three-tier architecture. |
| `Firmware/Prototype/CAN_Test_Arduino` | Legacy Arduino CAN prototype. | Prototype only; not part of current three-tier architecture. Owner clarification: can be archived; git history is enough if needed later. |
| `Firmware/Prototype/CAN_Test_Master` | Legacy/experimental STM32 CAN master prototype. | Prototype only; not current PanelBridge implementation. Owner clarification: can be archived; git history is enough if needed later. |
| `Firmware/Prototype/CAN_Test_SubNode` | Legacy/experimental STM32 CAN subnode prototype. | Prototype only; not current PanelGroup implementation. Owner clarification: can be archived; git history is enough if needed later. |
| `Firmware/Tests/*` | PlatformIO test projects for core libraries. | CI/test support projects, not deployed firmware images. |

**Compile checks run during recon:** `Firmware/Center_Armament`, the three `Firmware/Prototype/E2E_DCS_Test/*` projects, and the three `Firmware/Templates/*` projects all compile successfully with PlatformIO. This confirms buildability, not sim behavior.

**Architecture match:** The implementation matches the FirmwarePlan's three-tier architecture in the shared libraries: PanelGroup nodes own local I/O and CAN reports (`Firmware/Libraries/PanelGroup/PanelGroup.cpp:120-324`), PanelBridge owns DCS-BIOS integration and DCS/HID dispatch (`Firmware/Libraries/PanelBridge/PanelBridge.cpp:72-179`, `Firmware/Libraries/PanelBridge/PanelBridge.cpp:282-344`), and SimGateway is a byte relay plus USB HID endpoint, not a DCS-BIOS parser (`Firmware/Libraries/SimGateway/SimGateway.h:1-12`, `Firmware/Libraries/SimGateway/SimGateway.cpp:287-324`).

**E2E success criterion:** Owner clarification: the first full end-to-end sim test should validate the full lifecycle, not just a single control. It should include sending and receiving data from DCS, data traveling through the CAN network, and PanelBridge/SimGateway/PanelGroup intercepting or routing it as needed. Hardware scope is the `Firmware/Prototype/E2E_DCS_Test` sketches on breadboards only; no PCBs have been manufactured yet.

**Deviations or gaps:** Production `Center_Armament` has not yet been integrated beyond a stub. The current E2E PanelGroup uses PC13 as a dev-board LED shortcut (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:1-14`), while the hardware contract says PC13/PC14/PC15 are not production GPIO and production status LEDs should use PB14/PB15 (`Firmware/ScratchPad/FirmwarePlan/08-hardware-firmware-contracts.md:121-146`). The shared SimGateway library supports HID, but the current E2E SimGateway sketch is DCS-only and declares no HID controls (`Firmware/Prototype/E2E_DCS_Test/SimGateway/src/main.cpp:1-6`).

**Contracts not fully reflected yet:** Stepper-driver sleep/control, ADS1115-backed analog classes, stepper/servo outputs, and most planned input classes are specified but not implemented in firmware classes yet (`Firmware/ScratchPad/FirmwarePlan/08-hardware-firmware-contracts.md:55-93`, `Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:160-451`). The STM32 fixed-pin contract for CAN, status LEDs, diagnostic serial, and analog resolution is reflected in `STM32Board` (`Firmware/Libraries/STM32Board/STM32Board.h:9-44`, `Firmware/Libraries/STM32Board/STM32Board.cpp:71-117`). Owner clarification: the final stepper driver choice is not yet validated because stepper drivers have not been prototyped.

## 4. DCS-BIOS address coverage

**Completeness:** The generated A4EC headers are the current source of truth because they are generated from DCS-BIOS. The generated metadata currently has no generator gaps (`Firmware/Libraries/A4EC/GENERATOR_GAPS.md:7-9`), with 150 input controls and 313 output constants (`Firmware/Libraries/A4EC/A4EC_CmdIds.h:1-6`, `Firmware/Libraries/A4EC/A4EC_OutputIds.h:1-6`). `docs/claude/controllers/a4e-c-dcsbios-addresses.md` is a broad A-4E-C reference with additional context, but it should be treated as potentially stale until compared against the generated headers (`docs/claude/controllers/a4e-c-dcsbios-addresses.md:1-20`, `docs/claude/controllers/a4e-c-dcsbios-addresses.md:517-550`).

**Confirmed working vs researched vs placeholder:** DCS-BIOS-extracted addresses and masks are assumed valid. That is separate from "confirmed through OpenSkyhawk hardware/firmware E2E." The current E2E sketch exercises one output (`GEAR_LIGHT`) and one input (`MASTER_TEST`) (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:1-14`). Placeholder/TBD control details remain in panel docs, especially AWRS switch types and Misc panel notes (`docs/claude/controllers/AWRS_Panel.md:9-15`, `docs/claude/controllers/Misc_Switch_Panel.md:22-26`).

**Consistency with firmware:** Generated firmware constants agree with many documented controls. For example, `GEAR_LIGHT` is documented at address `0x845C` mask `0x8000` and generated as `A_4E_C_GEAR_LIGHT = 0x845c` plus `A_4E_C_GEAR_LIGHT_AM = 0x8000` (`docs/claude/controllers/a4e-c-dcsbios-addresses.md:117-138`, `Firmware/Libraries/A4EC/A4EC_OutputIds.h:629-630`). `MASTER_TEST` is documented at `0x8512` mask `0x0004` and generated as matching output constants, while its input command ID is `DCSIN_MASTER_TEST` (`docs/claude/controllers/a4e-c-dcsbios-addresses.md:296-305`, `Firmware/Libraries/A4EC/A4EC_OutputIds.h:737-738`, `Firmware/Libraries/A4EC/A4EC_CmdIds.h:119`).

**Armament Group mapping:** `docs/claude/controllers/Center_Armament.md` maps Armament Panel IDs 700-710 and Misc Switch Panel IDs 720-727 to DCS-BIOS names (`docs/claude/controllers/Center_Armament.md:103-136`, `docs/claude/controllers/Center_Armament.md:166-177`). The generated input map includes representative Armament/AWRS/Misc controls such as `ARM_MASTER`, `AWRS_QUANT`, `MISSILE_VOL`, `SHRIKE_SEL_KNB`, and `MASTER_TEST` (`Firmware/Libraries/A4EC/A4EC_InputMap.h:62-171`). AWRS docs still mark switch type details TBD (`docs/claude/controllers/AWRS_Panel.md:9-15`).

**Naming typo:** `_A` in the DCS-BIOS address reference and older examples is a typo/stale convention. The canonical generated output style is `A_4E_C_*` for the address and `A_4E_C_*_AM` for the address mask (`docs/claude/controllers/a4e-c-dcsbios-addresses.md:5-12`, `Firmware/ScratchPad/FirmwarePlan/04-dcs-bios-integration.md:41-56`, `Firmware/Libraries/A4EC/A4EC_OutputIds.h:13-18`, `Firmware/Libraries/A4EC/A4EC_OutputIds.h:629-630`).

## 5. Supported control types - confirmed vs planned

**Currently implemented in firmware source:**

| Type | How it is read/driven | Upstream/downstream report | Current use found |
| --- | --- | --- | --- |
| `PinRef` | GPIO, MCP23017 cached digital, or ADS1115 analog abstraction (`Firmware/Libraries/PanelGroup/PinRef.h:5-149`, `Firmware/Libraries/PanelGroup/PinRef.cpp:48-169`). | Helper only; no direct upstream report. | Used by `Switch2Pos` and `LED`. |
| `Switch2Pos` | Digital input through `PinRef`; debounced in `poll()`. MCP inputs can be interrupt-updated, then polled from cache; fallback polling runs every 20 ms (`Firmware/Libraries/PanelGroup/Switch2Pos.cpp:26-64`, `Firmware/Libraries/PanelGroup/PanelGroup.cpp:270-310`). | Sends batched `ControlPacket` on `canIdEvt(NODE_ID)`; PanelBridge routes by `controlId` to DCS-BIOS or HID (`Firmware/Libraries/PanelGroup/Switch2Pos.cpp:30-64`, `Firmware/Libraries/PanelBridge/PanelBridge.cpp:167-179`). | E2E PanelGroup uses it for `DCSIN_MASTER_TEST` (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:8-14`). |
| `LED` | Digital output through `PinRef`; matches DCS-BIOS output address/mask (`Firmware/Libraries/PanelGroup/LED.h:16-74`, `Firmware/Libraries/PanelGroup/LED.cpp:9-26`). | Consumes `CTRL_BCAST` output packets from PanelBridge (`Firmware/Libraries/PanelGroup/PanelGroup.cpp:79-92`). | E2E PanelGroup uses it for `A_4E_C_GEAR_LIGHT` (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:8-14`). |
| `HIDAxis`, `HIDButton`, `HIDHatSwitch` | SimGateway linked-list handlers receive HID frames from UART parser (`Firmware/Libraries/SimGateway/SimGateway.h:26-123`, `Firmware/Libraries/SimGateway/SimGateway.cpp:140-190`, `Firmware/Libraries/SimGateway/SimGateway.cpp:248-275`). | Updates TinyUSB HID report (`Firmware/Libraries/SimGateway/SimGateway.cpp:16-112`). | Implemented in library; not used by current DCS-only E2E sketch. |

**Referenced but not implemented yet:** Planned inputs include `Switch3Pos`, `SwitchMultiPos`, `AnalogMultiPos`, `ActionButton`, `RotaryEncoder`, `RotaryAcceleratedEncoder`, `RotarySwitch`, `AnalogInput`, `AngleSensorInput`, and `SwitchWithCover2Pos` (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:160-344`, `Firmware/ScratchPad/TechSpec/PanelGroup/Inputs/SwitchWithCover2Pos.md:3`). Planned outputs include `IntegerOutput`, `AnalogOutput`, `SwitecX25Output`, `AccelStepperOutput`, and `ServoOutput` (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:374-451`, `Firmware/ScratchPad/TechSpec/PanelGroup/Outputs/IntegerOutput.md:3`).

**Rotary encoders:** Rotary encoders are planned as incremental inputs with direction/speed handling. `RotaryEncoder` sends `DEC/INC` values, `RotaryAcceleratedEncoder` adds speed-based acceleration, and `RotarySwitch` handles absolute quadrature-style switches but has a boot ambiguity called out in the plan (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:222-280`).

**Multi-position handling:** Multi-position controls are planned as distinct classes rather than simple toggles: `SwitchMultiPos` uses one active-low pin per position, `AnalogMultiPos` uses an ADC resistor ladder, and `RotarySwitch` uses quadrature-like position encoding (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:174-210`, `Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:265-280`). PanelBridge already knows how to dispatch generated `MULTIPOS` and `ANALOG` input map entries, but PanelGroup source only has `Switch2Pos` implemented today (`Firmware/Libraries/PanelBridge/PanelBridge.cpp:80-165`).

## 6. SimGateway implementation

**Current state:** SimGateway is implemented as an RP2040 library and template/E2E firmware target. The library provides CDC-to-UART byte relay and HID demux; it does not run DCS-BIOS, parse DCS addresses, or talk CAN (`Firmware/Libraries/SimGateway/SimGateway.h:1-12`). The current E2E sketch is intentionally DCS-BIOS-only and declares no HID controls (`Firmware/Prototype/E2E_DCS_Test/SimGateway/src/main.cpp:1-6`).

**DCS vs HID distinction:** PanelBridge sends HID over UART as fixed 6-byte frames starting with `0xAA 0x55`, followed by little-endian `controlId` and value (`Firmware/ScratchPad/FirmwarePlan/03-uart-usb-hid-protocol.md:27-45`, `Firmware/Libraries/PanelBridge/PanelBridge.cpp:63-70`). SimGateway treats non-`0xAA` bytes as raw DCS-BIOS bytes and forwards them to USB CDC; `0xAA 0x55` starts HID frame parsing; `0xAA` followed by anything else is forwarded/resynced (`Firmware/Libraries/SimGateway/SimGateway.cpp:225-275`).

**TinyUSB classes:** The production path uses a composite USB device: CDC serial plus TinyUSB HID report via `Adafruit_USBD_HID` (`Firmware/ScratchPad/FirmwarePlan/03-uart-usb-hid-protocol.md:76-91`, `Firmware/Libraries/SimGateway/SimGateway.cpp:11-13`, `Firmware/Libraries/SimGateway/SimGateway.cpp:23-86`).

**UART baud/protocol:** UART between SimGateway and PanelBridge is 250000 baud on RP2040 default pins GP0 TX / GP1 RX (`Firmware/ScratchPad/FirmwarePlan/03-uart-usb-hid-protocol.md:10-23`, `Firmware/Libraries/SimGateway/SimGateway.h:127-149`, `Firmware/Libraries/SimGateway/SimGateway.cpp:287-309`). SimGateway-to-PanelBridge carries raw DCS-BIOS binary bytes; PanelBridge-to-SimGateway carries raw DCS-BIOS bytes plus HID frames (`Firmware/ScratchPad/FirmwarePlan/03-uart-usb-hid-protocol.md:15-23`).

**Seven HID axes:** The canonical planned HID axes are roll, pitch, throttle, rudder, left brake, right brake, and zoom (`Firmware/Libraries/HIDControls/HIDControls.h:21-28`). Speed brake is not an axis. FirmwarePlan also lists the same seven source constants as roll, pitch, throttle, rudder, brake left, brake right, and zoom (`Firmware/ScratchPad/FirmwarePlan/07-simgateway-api.md:24-42`).

## 7. PCB and schematic status

**KiCad projects found under `PCB/`:**

| Path | Description | Status found |
| --- | --- | --- |
| `PCB/Center_Console/Center_Armament/Armament_MCU` | Main MCU/CAN board for Armament group, with sheets for MCU/CAN, backlighting, gauges, and switches (`PCB/Center_Console/Center_Armament/Armament_MCU/Armament_MCU.kicad_sch:8-11`, `PCB/Center_Console/Center_Armament/Armament_MCU/Armament_MCU.kicad_sch:240-529`). | Real/in progress. ERC is not clean: 2 errors for unconnected U1 PB10/PB11 and 2 unconnected-wire warnings. No board file found for DRC in this project. |
| `PCB/Center_Console/Center_Armament/AWRS_Panel` | AWRS breakout/control board with controls and backlighting sheets (`PCB/Center_Console/Center_Armament/AWRS_Panel/AWRS_Panel.kicad_sch:8`, `PCB/Center_Console/Center_Armament/AWRS_Panel/AWRS_Panel.kicad_sch:5098-5155`). | Real/in progress. ERC is not clean: 1 error for SW1 pin 13 not connected and 23 off-grid warnings. |
| `PCB/Center_Console/Center_Armament/Misc_Switch_Panel` | Misc Switch Panel breakout with backlight/switch sheets and PCB (`PCB/Center_Console/Center_Armament/Misc_Switch_Panel/Misc_Switch_Panel.kicad_sch:7551-7628`). | ERC clean with 0 violations. DRC could not be obtained during recon because `kicad-cli pcb drc` aborted with exit 134 and produced no report. |

**Armament Group schematic completion:** All three Armament Group boards have real schematics rather than stubs. `Misc_Switch_Panel` is ERC-clean; `Armament_MCU` and `AWRS_Panel` are still in-progress because ERC reports errors/warnings. Owner clarification: PCB work should be treated as schematic exploration/in-progress and not validated by fabrication. Detailed progress is tracked internally for now; public tracking may later move to GitHub Issues or Projects after the format is decided.

**Shared symbols/footprints:** `PCB/Libraries/OpenSkyhawk.kicad_sym` contains project-specific symbols such as `IRLML2502`, `LED_5050_Red`, `X27.168`, and `X27.589` (`PCB/Libraries/OpenSkyhawk.kicad_sym:5-25`, `PCB/Libraries/OpenSkyhawk.kicad_sym:304-355`, `PCB/Libraries/OpenSkyhawk.kicad_sym:502-533`, `PCB/Libraries/OpenSkyhawk.kicad_sym:824-855`). Matching footprints exist under `PCB/Libraries/OpenSkyhawk.pretty/`, including `LED_5050_Red.kicad_mod`, `X27.168_Stepper.kicad_mod`, and `X27.589_Stepper.kicad_mod`.

**KiCad template:** A project template exists under `PCB/Libraries/project-template/`. It uses project-local library tables pointing to `${KIPRJMOD}/../../../Libraries/...` (`PCB/Libraries/project-template/sym-lib-table:3`, `PCB/Libraries/project-template/fp-lib-table:3`) and preloads JLCPCB-oriented design rules, library pins, net classes, and auto net-class patterns (`PCB/Libraries/project-template/template.kicad_pro:52-68`, `PCB/Libraries/project-template/template.kicad_pro:143-148`, `PCB/Libraries/project-template/template.kicad_pro:205-274`). `jlcpcb-standard.kicad_dru` also exists (`PCB/Libraries/design-rules/jlcpcb-standard.kicad_dru:1-45`).

**Template-sheet gap:** `PCB/Libraries/sheets/README.md` documents reusable sheet templates (`PCB/Libraries/sheets/README.md:3-23`), but no `.kicad_sch` sheet template files were found in that directory. Owner clarification: several KiCad templates are still needed, probably one per board type; the PanelGroup template is the highest priority.

## 8. Repo structure - anything unexpected

**Top-level structure:** The root includes the expected `CAD/`, `PCB/`, `Firmware/`, and `docs/` folders described in the architecture overview (`docs/claude/architecture.md:1-9`). Additional top-level entries found include `.github/`, `.claude/`, `.claire/`, `.pytest_cache/`, `AGENTS.md`, `CLAUDE.md`, `CODE_OF_CONDUCT.md`, `OpenSkyhawk.code-workspace`, `site/`, and `tools/`.

**CAD status:** The committed `CAD/` tree currently contains only console/shared placeholder directories with `.gitkeep` files. Owner clarification: CAD work is still very early, with only a couple of local panel sketches so far. Future docs should not imply CAD models are mature or complete.

**Docs mismatch:** The root README describes the older high-level structure but does not account for current `Firmware/Prototype`, `Firmware/Templates`, `Firmware/Tests`, `.github`, `tools`, or `site` folders (`README.md:12-47`). The architecture overview is closer to the current structure but intentionally summarizes only the major discipline folders (`docs/claude/architecture.md:1-39`).

**GitHub Actions:** `.github/workflows/` exists. Docs-specific workflows include:

- `docs-deploy.yml`: on pushes to `main` affecting docs/Firmware libraries/workflow files, installs MkDocs dependencies and publishes with `mkdocs gh-deploy -f docs/mkdocs.yml --force --remote-branch gh-pages` (`.github/workflows/docs-deploy.yml:1-41`).
- `docs-ci.yml`: PR docs checks, including Markdown link validation while excluding generated API docs (`.github/workflows/docs-ci.yml:1-91`).
- Additional CI exists for firmware, KiCad, A4EC metadata refresh/generator checks, and PR hygiene (`.github/workflows/firmware-ci.yml:1-107`, `.github/workflows/kicad-ci.yml:1-117`, `.github/workflows/a4ec-ci.yml:1-84`, `.github/workflows/a4ec-refresh.yml:1-233`, `.github/workflows/pr-hygiene.yml:1-46`).

**Root conduct/contributing files:** `CODE_OF_CONDUCT.md` exists at the root. `CONTRIBUTING.md` was not found.

**Unexpected workflow issue:** `pr-hygiene.yml` refers to `find Tools`, but the repo folder is `tools`, lower-case (`.github/workflows/pr-hygiene.yml:38-46`). Owner clarification: `tools` is the correct path, so the workflow should be updated to lowercase.

## 9. Docs site configuration

**Site/repo URLs:** `docs/mkdocs.yml` sets `site_url` to `https://openskyhawk.github.io/OpenSkyhawk/` and `repo_url` to `https://github.com/OpenSkyhawk/OpenSkyhawk` (`docs/mkdocs.yml:1-6`).

**Plugins:** The configured plugins are `search` and `mkdoxy`; no additional plugins beyond mkdoxy/search were found (`docs/mkdocs.yml:88-109`).

**Theme overrides/hooks:** The Material theme uses `custom_dir: overrides`, and `docs/overrides/partials/header.html` defines a custom header/nav/search/source layout (`docs/mkdocs.yml:8-19`, `docs/overrides/partials/header.html:1-72`). No MkDocs hooks entry was found in `mkdocs.yml`.

**Current nav:** `mkdocs.yml` currently includes Home, Architecture pages, Controller pages, and generated API Reference pages (`docs/mkdocs.yml:24-66`). `not_in_nav` and `exclude_docs` suppress scratch/reference/generated inputs from nav/build (`docs/mkdocs.yml:67-87`).

**Proposed nav comparison:** The provided proposed nav is an aspirational replacement `nav:` block, not a standalone `mkdocs-nav.yml` file and not a drop-in change today (`/Users/mhoresh/.codex/attachments/59d5d575-5112-40e4-abf9-97eeba9edc8d/pasted-text.txt:1-6`). It reorganizes the site into reader-journey sections: Getting Started, Architecture, Hardware Reference, Firmware Reference, console/panel pages, HOTAS, Kit Assembly, Build Guides, Contributing, and API Reference (`/Users/mhoresh/.codex/attachments/59d5d575-5112-40e4-abf9-97eeba9edc8d/pasted-text.txt:8-114`). Owner clarification: keep the current nested API Reference structure to support the generated API docs format. Kit Assembly should be planned as a future docs category, but should not imply kits are currently available. Kits are expected to be released incrementally as each panel group is completed; Armament Group is the first intended target if E2E and follow-on validation go well.

**Audience guidance:** Owner clarification: current docs should target builders and contributors first, while being clear and polished enough to attract people who may want to contribute to the project.

**Future platform direction:** Owner clarification: OpenSkyhawk is A-4E-focused today, but the firmware/hardware architecture could be useful for other DCS cockpit projects later. A possible future generic kit could split into two boards: a basic PanelGroup essentials board exposing all open pins, and a custom STM32F103 MCU/bridge board with a pluggable RP2040 module to avoid soldering the RP2040 directly. This is conceptual future direction and should remain private/internal until developed, though it may be prototyped early because it would help validate more complex networks.

**Migration risk:** Aside from `index.md` and `api/index.md`, the proposed target paths live under new directories that do not currently exist, such as `getting-started/`, `architecture/`, `hardware/`, `firmware/`, `panels/`, `flight-controls/`, `kits/`, `guides/`, and `contributing/` (`/Users/mhoresh/.codex/attachments/59d5d575-5112-40e4-abf9-97eeba9edc8d/pasted-text.txt:8-114`). If applied as-is, MkDocs strict builds would fail until content is moved or redirects/stubs are created. Current docs can be mapped conceptually, but not by path:

- `claude/architecture.md` likely becomes `architecture/index.md` or `getting-started/system-overview.md`.
- `claude/hardware-standards.md`, `claude/pcb-design-rules.md`, and `claude/kicad.md` likely move under `hardware/`.
- `claude/dcsbios-stm32-debug.md` likely becomes `firmware/debugging.md`.
- `claude/docs-site-workflow.md` likely becomes `contributing/docs-workflow.md`.
- `claude/controllers/Center_Armament.md`, `AWRS_Panel.md`, and `Misc_Switch_Panel.md` likely split into the proposed Armament Group pages (`/Users/mhoresh/.codex/attachments/59d5d575-5112-40e4-abf9-97eeba9edc8d/pasted-text.txt:39-48`).
- `claude/controllers/a4e-c-dcsbios-addresses.md` has no direct proposed nav entry. Since it may be stale relative to generated A4EC headers, compare it against `A4EC_CmdIds.h`, `A4EC_InputMap.h`, and `A4EC_OutputIds.h`; keep only matching/additional context in the future docs.
- The current API nav exposes generated subsections for namespaces, classes, files, modules, and pages (`docs/mkdocs.yml:37-66`). Preserve that nested API structure even if the rest of the proposed nav migrates.

**Generated site note:** `docs/claude/docs-site-workflow.md` says `site/` is generated output and should not be committed (`docs/claude/docs-site-workflow.md:15-23`, `docs/claude/docs-site-workflow.md:51-59`). Owner clarification: a local `site/` folder is expected for preview and is gitignored; GitHub Actions generates and publishes the actual site to the publishing branch.

## 10. Open questions to flag

- **Switch2Pos status is stale in FirmwarePlan.** FirmwarePlan says it needs a `PinRef` update/deferred work, but TechSpec and source show it is done (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:151-158`, `Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:129-132`, `Firmware/ScratchPad/TechSpec/PanelGroup/Inputs/Switch2Pos.md:3`, `Firmware/Libraries/PanelGroup/Switch2Pos.cpp:14-64`).
- **IntegerOutput is planned, not implemented.** FirmwarePlan and `PanelGroup/README.md` are stale where they say/show it as existing; TechSpec and source search confirm no OpenSkyhawk implementation class yet (`Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:374-381`, `Firmware/Libraries/PanelGroup/README.md:37-60`, `Firmware/ScratchPad/TechSpec/PanelGroup/Outputs/IntegerOutput.md:3`).
- **A4EC generator status is stale.** FirmwarePlan says 149 inputs and one unsupported paired boolean control; current generated metadata says 150 controls and no generator gaps (`Firmware/ScratchPad/FirmwarePlan/10-implementation-plan.md:65-66`, `Firmware/Libraries/A4EC/A4EC_CmdIds.h:1-6`, `Firmware/Libraries/A4EC/GENERATOR_GAPS.md:7-9`).
- **FirmwarePlan should be corrected, not merely archived.** It is internal planning material, but future docs will be derived from it, so conflicts with TechSpec/source should be resolved in FirmwarePlan.
- **HID range is standardized on the broader envelope.** Source/docs now reserve `0x0010-0x00FF` as the HID routing envelope, with `0x00B0-0x00FF` reserved for future expansion rather than current USB button slots (`Firmware/Libraries/HIDControls/HIDControls.h:8-45`, `Firmware/ScratchPad/FirmwarePlan/04-dcs-bios-integration.md:10-20`).
- **`_A` is a typo/stale suffix.** Standardize docs on generated `A_4E_C_*` address constants plus `A_4E_C_*_AM` masks; update old `_A` examples (`docs/claude/controllers/a4e-c-dcsbios-addresses.md:5-12`, `Firmware/ScratchPad/FirmwarePlan/04-dcs-bios-integration.md:41-56`, `Firmware/Libraries/A4EC/A4EC_OutputIds.h:13-18`, `Firmware/ScratchPad/FirmwarePlan/05-panelgroup-api.md:366-398`).
- **`PanelGroup/README.md` appears stale.** It still refers to PA0 node strap behavior, `IntegerOutput`, and older `sendEvent`/`nodeId` style APIs, while current code uses compile-time `NODE_ID`, `PinRef`, and `CANProtocol::sendBatched` (`Firmware/Libraries/PanelGroup/README.md:20-37`, `Firmware/Libraries/PanelGroup/README.md:60-64`, `Firmware/Libraries/PanelGroup/README.md:90-93`, `Firmware/Libraries/PanelGroup/Switch2Pos.cpp:36-62`).
- **Center Armament firmware is not integrated yet.** The production sketch compiles but is stub-only, and the controller docs still have CAN node ID TBD (`Firmware/Center_Armament/src/main.cpp:1-12`, `docs/claude/controllers/Center_Armament.md:13`).
- **Legacy CAN prototype projects can be archived.** `CAN_Test_Arduino`, `CAN_Test_Master`, and `CAN_Test_SubNode` predate the current firmware architecture and do not need user-facing docs coverage; git history is sufficient for recovery if needed.
- **E2E LED pin is prototype-only.** The E2E PanelGroup uses PC13 for a dev-board LED, while the hardware contract says PC13/PC14/PC15 are not production GPIO (`Firmware/Prototype/E2E_DCS_Test/PanelGroup/src/main.cpp:1-14`, `Firmware/ScratchPad/FirmwarePlan/08-hardware-firmware-contracts.md:121-146`).
- **CAD is very early.** The committed `CAD/` tree is placeholder-only; only a couple local panel sketches exist so far, so future docs should present CAD as planned/early.
- **Reusable KiCad sheet templates are planned but absent.** `PCB/Libraries/sheets/README.md` lists available templates, but no `.kicad_sch` templates were found in that directory. Several templates are needed, likely one per board type, with PanelGroup as the priority (`PCB/Libraries/sheets/README.md:3-23`).
- **`pr-hygiene.yml` has a case typo.** It checks `Tools` but the correct repo path is lowercase `tools` (`.github/workflows/pr-hygiene.yml:38-46`).
- **Stepper driver choice is not yet validated.** Docs currently conflict between DRV8835 and DRV8833PW wording, but the final driver should remain TBD until stepper-driver prototypes are tested (`docs/claude/hardware-standards.md:75-95`, `docs/claude/kicad.md:31-42`, `docs/claude/pcb-design-rules.md:43-149`).
- **The proposed MkDocs nav is aspirational, not drop-in.** It points mostly at new paths that do not exist yet; keep the current nested API Reference structure, and compare `a4e-c-dcsbios-addresses.md` against generated A4EC headers before deciding what context to carry forward (`docs/mkdocs.yml:24-66`, `/Users/mhoresh/.codex/attachments/59d5d575-5112-40e4-abf9-97eeba9edc8d/pasted-text.txt:8-114`).
- **Misc Switch Panel DRC is unknown.** ERC is clean, but KiCad CLI DRC aborted with exit 134 and produced no usable report.
