
# Class List


Here are the classes, structs, unions and interfaces with brief descriptions:

* **class** [**ADS1115**](classADS1115.md) [_**ADS1115**_](classADS1115.md) _ADC. Inherits Adafruit\_ADS1115 without modification._
* **struct** [**BatchState**](structBatchState.md)     
* **namespace** [**CANProtocol**](namespaceCANProtocol.md)     
* **namespace** [**DcsBios**](namespaceDcsBios.md)     
* **struct** [**DcsBiosInputEntry**](structDcsBiosInputEntry.md)     
* **namespace** [**InputType**](namespaceInputType.md)     
* **namespace** [**OpenSkyhawk**](namespaceOpenSkyhawk.md)     
    * **struct** [**AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) _One point on the acceleration curve (SwitecX25 form)._     
    * **class** [**AnalogMultiPos**](classOpenSkyhawk_1_1AnalogMultiPos.md) _Resistor-ladder multi-position selector — one analog_ `PinRef` _, a different voltage per position. Emits the resolved position index 0..N-1 over CAN (MULTIPOS dispatch)._    
    * **class** [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) _Rolling-drum OLED readout. One instance == one OLED panel._     
    * **struct** [**DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) _Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._     
    * **struct** [**DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) _A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._     
    * **struct** [**DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) _Complete description of one rolling readout: its sources, geometry, glyphs, flag._     
    * **struct** [**DrumSource**](structOpenSkyhawk_1_1DrumSource.md) _One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._    
    * **struct** [**GaugeCal**](structOpenSkyhawk_1_1GaugeCal.md) _Value → position calibration for one gauge._     
    * **class** [**HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) _HID axis handler. Declared at sketch scope for each joystick axis._     
    * **class** [**HIDButton**](classOpenSkyhawk_1_1HIDButton.md) _HID button handler. Declared at sketch scope for each button._     
    * **class** [**HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) _HID hat switch handler. Declared at sketch scope for each hat switch._     
    * **struct** [**HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) _Home-sensor parameters (_ HomeMode::SENSOR _only)._    
    * **class** [**I2cMux**](classOpenSkyhawk_1_1I2cMux.md) _Selects one downstream channel of a TCA9548A I2C multiplexer._     
    * **class** [**InputBase**](classOpenSkyhawk_1_1InputBase.md) _Abstract base for all hardware-polled input objects._     
    * **class** [**LED**](classOpenSkyhawk_1_1LED.md) _Digital_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output. Drives a pin based on a DCS-BIOS state value._    
    * **class** [**MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md) _Common interface every motor/servo backend implements._     
    * **class** [**MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md) _Base for the MULTIPOS input family — selectors that emit an absolute position index 0..N-1 over CAN. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._    
    * **class** [**NeedleGauge**](classOpenSkyhawk_1_1NeedleGauge.md) _DCS-driven pointer gauge over any_ [_**MotorDriver**_](classOpenSkyhawk_1_1MotorDriver.md) _backend._    
    * **class** [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) _Abstract base for all DCS-driven output objects._     
    * **struct** [**StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) _Full per-instance stepper configuration. Authored per sketch (panel wiring)._     
    * **class** [**StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md) _Non-blocking instrument-gauge stepper driven through_ [_**PinRef**_](classPinRef.md) _coils._    
    * **class** [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) _Debounced 2-position switch. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._    
    * **class** [**Switch3Pos**](classOpenSkyhawk_1_1Switch3Pos.md) _Three-position switch (ON-OFF-ON / spring-centred) on two pins. Emits 0 / 1 / 2 over CAN (MULTIPOS dispatch)._     
    * **class** [**SwitchMultiPos**](classOpenSkyhawk_1_1SwitchMultiPos.md) _Multi-position rotary selector — N discrete pins, exactly one active at a time. Emits the active position index 0..N-1 over CAN (MULTIPOS dispatch)._     
* **namespace** [**OpenSkyhawk**](namespaceOpenSkyhawk_1_1_0d12.md) 
* **namespace** [**PanelBridge**](namespacePanelBridge.md)     
* **namespace** [**PanelGroup**](namespacePanelGroup.md) _Static singleton for CAN sub-node (_ [_**PanelGroup**_](namespacePanelGroup.md) _) firmware._    
* **class** [**PinRef**](classPinRef.md) _Hardware pin abstraction used by all_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _input and output classes._    
* **struct** [**RxQueueEntry**](structRxQueueEntry.md)     
* **namespace** [**STM32Board**](namespaceSTM32Board.md)     
* **namespace** [**SimGateway**](namespaceSimGateway.md)     
* **struct** [**TxQueueEntry**](structTxQueueEntry.md)     
* **namespace** [**anonymous namespace{Firmware/Libraries/PanelBridge/PanelBridge.cpp}**](namespace_0d8.md) 
* **namespace** [**anonymous namespace{Firmware/Libraries/PanelGroup/PanelGroup.cpp}**](namespace_0d31.md) 
* **namespace** [**anonymous namespace{Firmware/Libraries/SimGateway/SimGateway.cpp}**](namespace_0d35.md)     

