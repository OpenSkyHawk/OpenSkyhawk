
# Class Hierarchy

This inheritance list is sorted roughly, but not completely, alphabetically:


* **class** [**OpenSkyhawk::InputBase**](classOpenSkyhawk_1_1InputBase.md) _Abstract base for all hardware-polled input objects._     
    * **class** [**OpenSkyhawk::MultiPosInput**](classOpenSkyhawk_1_1MultiPosInput.md) _Base for the MULTIPOS input family — selectors that emit an absolute position index 0..N-1 over CAN. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._    
        * **class** [**OpenSkyhawk::AnalogMultiPos**](classOpenSkyhawk_1_1AnalogMultiPos.md) _Resistor-ladder multi-position selector — one analog_ `PinRef` _, a different voltage per position. Emits the resolved position index 0..N-1 over CAN (MULTIPOS dispatch)._
        * **class** [**OpenSkyhawk::SwitchMultiPos**](classOpenSkyhawk_1_1SwitchMultiPos.md) _Multi-position rotary selector — N discrete pins, exactly one active at a time. Emits the active position index 0..N-1 over CAN (MULTIPOS dispatch)._ 
    * **class** [**OpenSkyhawk::Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) _Debounced 2-position switch. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._
* **class** [**OpenSkyhawk::OutputBase**](classOpenSkyhawk_1_1OutputBase.md) _Abstract base for all DCS-driven output objects._     
    * **class** [**OpenSkyhawk::DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) _Rolling-drum OLED readout. One instance == one OLED panel._ 
    * **class** [**OpenSkyhawk::LED**](classOpenSkyhawk_1_1LED.md) _Digital_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output. Drives a pin based on a DCS-BIOS state value._
    * **class** [**OpenSkyhawk::NeedleGauge**](classOpenSkyhawk_1_1NeedleGauge.md) _DCS-driven pointer gauge over any_ [_**MotorDriver**_](classOpenSkyhawk_1_1MotorDriver.md) _backend._
* **class** [**OpenSkyhawk::HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) _HID axis handler. Declared at sketch scope for each joystick axis._ 
* **class** [**OpenSkyhawk::HIDButton**](classOpenSkyhawk_1_1HIDButton.md) _HID button handler. Declared at sketch scope for each button._ 
* **class** [**OpenSkyhawk::HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) _HID hat switch handler. Declared at sketch scope for each hat switch._ 
* **class** [**OpenSkyhawk::I2cMux**](classOpenSkyhawk_1_1I2cMux.md) _Selects one downstream channel of a TCA9548A I2C multiplexer._ 
* **class** [**OpenSkyhawk::MotorDriver**](classOpenSkyhawk_1_1MotorDriver.md) _Common interface every motor/servo backend implements._     
    * **class** [**OpenSkyhawk::StepperMotor**](classOpenSkyhawk_1_1StepperMotor.md) _Non-blocking instrument-gauge stepper driven through_ [_**PinRef**_](classPinRef.md) _coils._
* **class** [**PinRef**](classPinRef.md) _Hardware pin abstraction used by all_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _input and output classes._
* **struct** [**BatchState**](structBatchState.md) 
* **struct** [**DcsBiosInputEntry**](structDcsBiosInputEntry.md) 
* **struct** [**OpenSkyhawk::AccelPoint**](structOpenSkyhawk_1_1AccelPoint.md) _One point on the acceleration curve (SwitecX25 form)._ 
* **struct** [**OpenSkyhawk::DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) _Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._ 
* **struct** [**OpenSkyhawk::DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) _A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._ 
* **struct** [**OpenSkyhawk::DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) _Complete description of one rolling readout: its sources, geometry, glyphs, flag._ 
* **struct** [**OpenSkyhawk::DrumSource**](structOpenSkyhawk_1_1DrumSource.md) _One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._
* **struct** [**OpenSkyhawk::GaugeCal**](structOpenSkyhawk_1_1GaugeCal.md) _Value → position calibration for one gauge._ 
* **struct** [**OpenSkyhawk::HomeSensor**](structOpenSkyhawk_1_1HomeSensor.md) _Home-sensor parameters (_ HomeMode::SENSOR _only)._
* **struct** [**OpenSkyhawk::StepperConfig**](structOpenSkyhawk_1_1StepperConfig.md) _Full per-instance stepper configuration. Authored per sketch (panel wiring)._ 
* **struct** [**RxQueueEntry**](structRxQueueEntry.md) 
* **struct** [**TxQueueEntry**](structTxQueueEntry.md) 
* **class** **Adafruit_ADS1115**    
    * **class** [**ADS1115**](classADS1115.md) [_**ADS1115**_](classADS1115.md) _ADC. Inherits Adafruit\_ADS1115 without modification._

