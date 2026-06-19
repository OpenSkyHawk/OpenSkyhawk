
# Class Hierarchy

This inheritance list is sorted roughly, but not completely, alphabetically:


* **class** [**OpenSkyhawk::OutputBase**](classOpenSkyhawk_1_1OutputBase.md) _Abstract base for all DCS-driven output objects._     
    * **class** [**OpenSkyhawk::DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) _Rolling-drum OLED readout. One instance == one OLED panel._ 
    * **class** [**OpenSkyhawk::LED**](classOpenSkyhawk_1_1LED.md) _Digital_ [_**LED**_](classOpenSkyhawk_1_1LED.md) _output. Drives a pin based on a DCS-BIOS state value._
* **class** [**OpenSkyhawk::HIDAxis**](classOpenSkyhawk_1_1HIDAxis.md) _HID axis handler. Declared at sketch scope for each joystick axis._ 
* **class** [**OpenSkyhawk::HIDButton**](classOpenSkyhawk_1_1HIDButton.md) _HID button handler. Declared at sketch scope for each button._ 
* **class** [**OpenSkyhawk::HIDHatSwitch**](classOpenSkyhawk_1_1HIDHatSwitch.md) _HID hat switch handler. Declared at sketch scope for each hat switch._ 
* **class** [**OpenSkyhawk::I2cMux**](classOpenSkyhawk_1_1I2cMux.md) _Selects one downstream channel of a TCA9548A I2C multiplexer._ 
* **class** [**OpenSkyhawk::InputBase**](classOpenSkyhawk_1_1InputBase.md) _Abstract base for all hardware-polled input objects._     
    * **class** [**OpenSkyhawk::Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) _Debounced 2-position switch. Self-registers into_ [_**PanelGroup**_](namespacePanelGroup.md) _'s_[_**InputBase**_](classOpenSkyhawk_1_1InputBase.md) _list._
* **class** [**PinRef**](classPinRef.md) _Hardware pin abstraction used by all_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _input and output classes._
* **struct** [**BatchState**](structBatchState.md) 
* **struct** [**DcsBiosInputEntry**](structDcsBiosInputEntry.md) 
* **struct** [**OpenSkyhawk::DrumFlag**](structOpenSkyhawk_1_1DrumFlag.md) _Optional 2-state (or N-state) flag tape — hemisphere N/S · E/W, or a mode letter._ 
* **struct** [**OpenSkyhawk::DrumGlyph**](structOpenSkyhawk_1_1DrumGlyph.md) _A fixed (non-rolling) glyph painted between digit columns — '.', ' ', ':' etc._ 
* **struct** [**OpenSkyhawk::DrumReadout**](structOpenSkyhawk_1_1DrumReadout.md) _Complete description of one rolling readout: its sources, geometry, glyphs, flag._ 
* **struct** [**OpenSkyhawk::DrumSource**](structOpenSkyhawk_1_1DrumSource.md) _One DCS-BIOS digit source feeding a_ [_**DrumReadout**_](structOpenSkyhawk_1_1DrumReadout.md) _._
* **struct** [**RxQueueEntry**](structRxQueueEntry.md) 
* **struct** [**TxQueueEntry**](structTxQueueEntry.md) 
* **class** **Adafruit_ADS1115**    
    * **class** [**ADS1115**](classADS1115.md) [_**ADS1115**_](classADS1115.md) _ADC. Inherits Adafruit\_ADS1115 without modification._

