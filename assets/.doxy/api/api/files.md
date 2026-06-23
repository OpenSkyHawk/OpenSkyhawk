
# File List

Here is a list of all files with brief descriptions:


* **dir** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md)     
    * **dir** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md)     
        * **dir** [**A4EC**](dir_4c3761349dd87c66ada20ca5eb39042a.md)     
            * **file** [**A4EC\_CmdIds.h**](A4EC__CmdIds_8h.md)     
            * **file** [**A4EC\_InputMap.h**](A4EC__InputMap_8h.md)     
            * **file** [**A4EC\_OutputIds.h**](A4EC__OutputIds_8h.md)     
        * **dir** [**CANProtocol**](dir_81ff3032570f78b12938068450b63228.md)     
            * **file** [**CANProtocol.cpp**](CANProtocol_8cpp.md)     
            * **file** [**CANProtocol.h**](CANProtocol_8h.md) _Shared CAN bus types, frame IDs, and runtime API for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _STM32 nodes._    
        * **dir** [**DrumDisplay**](dir_b8ac8c4bf654b399fff34fe5003d7d6c.md)     
            * **file** [**DrumDisplay.cpp**](DrumDisplay_8cpp.md)     
            * **file** [**DrumDisplay.h**](DrumDisplay_8h.md) _Rolling mechanical-drum OLED readout output for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._    
        * **dir** [**HIDControls**](dir_8de7ffd664ed88ad14416481a318893f.md)     
            * **file** [**HIDControls.h**](HIDControls_8h.md) _CAN controlId constants for HID axes and buttons._     
        * **dir** [**PanelBridge**](dir_f592a3c441b32532ba8eb6b28add2a90.md)     
            * **file** [**PanelBridge.cpp**](PanelBridge_8cpp.md)     
            * **file** [**PanelBridge.h**](PanelBridge_8h.md) _STM32 CAN master and DCS-BIOS processing node for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _._    
        * **dir** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md)     
            * **file** [**ADS1115.h**](ADS1115_8h.md) _Thin wrapper over Adafruit\_ADS1115 that provides a forward-declarable class name._     
            * **dir** [**Drivers**](dir_da1b6a20235952b69490534d482f5898.md)     
                * **dir** [**MotorDriver**](dir_7cabaf4812e32c14ff26922d3804a645.md)     
                    * **file** [**MotorDriver.h**](MotorDriver_8h.md) _Abstract base for non-blocking motor/servo drivers._     
                * **dir** [**StepperMotor**](dir_f431add5022471a872df403ed217c535.md)     
                    * **file** [**StepperMotor.cpp**](StepperMotor_8cpp.md)     
                    * **file** [**StepperMotor.h**](StepperMotor_8h.md) _Non-blocking 4-wire stepper driver on_ [_**PinRef**_](classPinRef.md) _coils._    
            * **dir** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md)     
                * **dir** [**I2cMux**](dir_b0e3ddf276daac85bddb20c46644a5c8.md)     
                    * **file** [**I2cMux.cpp**](I2cMux_8cpp.md)     
                    * **file** [**I2cMux.h**](I2cMux_8h.md) _TCA9548A 1-to-8 I2C multiplexer channel selector for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _._    
            * **dir** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md)     
                * **dir** [**AnalogMultiPos**](dir_869066037a4dfe81b59e09c740cc62d3.md)     
                    * **file** [**AnalogMultiPos.cpp**](AnalogMultiPos_8cpp.md)     
                    * **file** [**AnalogMultiPos.h**](AnalogMultiPos_8h.md) _Resistor-ladder multi-position selector for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._    
                * **dir** [**MultiPosInput**](dir_7bc1eaced50854697a5557e9b0a7cd3c.md)     
                    * **file** [**MultiPosInput.cpp**](MultiPosInput_8cpp.md)     
                    * **file** [**MultiPosInput.h**](MultiPosInput_8h.md) _Shared base for multi-position selector inputs (SwitchMultiPos, AnalogMultiPos, ...)._     
                * **dir** [**Switch2Pos**](dir_498c816b3a939baf976ad59345a9b3b2.md)     
                    * **file** [**Switch2Pos.cpp**](Switch2Pos_8cpp.md)     
                    * **file** [**Switch2Pos.h**](Switch2Pos_8h.md) _Debounced 2-position switch for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._    
                * **dir** [**Switch3Pos**](dir_344ec6ab1ff9af7411f07538fed6b206.md)     
                    * **file** [**Switch3Pos.cpp**](Switch3Pos_8cpp.md)     
                    * **file** [**Switch3Pos.h**](Switch3Pos_8h.md) _Three-position (ON-OFF-ON) switch for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._    
                * **dir** [**SwitchMultiPos**](dir_4dc253f801dfeffbf99c560a0635ade6.md)     
                    * **file** [**SwitchMultiPos.cpp**](SwitchMultiPos_8cpp.md)     
                    * **file** [**SwitchMultiPos.h**](SwitchMultiPos_8h.md) _N-pin rotary selector switch for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._    
            * **dir** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md)     
                * **dir** [**LED**](dir_014b7653223add72b0ed2d7a88fd1566.md)     
                    * **file** [**LED.cpp**](LED_8cpp.md) 
                    * **file** [**LED.h**](LED_8h.md) _Digital LED output for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**PanelGroup**_](namespacePanelGroup.md) _nodes._    
                * **dir** [**NeedleGauge**](dir_61ced45d99aac20e353c7cae873553bb.md)     
                    * **file** [**NeedleGauge.cpp**](NeedleGauge_8cpp.md) 
                    * **file** [**NeedleGauge.h**](NeedleGauge_8h.md) _Pointer-gauge output: maps one DCS-BIOS value to a motor position._     
            * **file** [**OpenSkyhawk.h**](OpenSkyhawk_8h.md) _Umbrella include for_ [_**PanelGroup**_](namespacePanelGroup.md) _sketch files._
            * **file** [**PanelGroup.cpp**](PanelGroup_8cpp.md)     
            * **file** [**PanelGroup.h**](PanelGroup_8h.md) _CAN sub-node domain layer for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel boards._    
            * **file** [**PinRef.cpp**](PinRef_8cpp.md)     
            * **file** [**PinRef.h**](PinRef_8h.md) _Hardware pin abstraction for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel controls._    
        * **dir** [**STM32Board**](dir_aa1816754c0645981f9c7af905857f7d.md)     
            * **file** [**STM32Board.cpp**](STM32Board_8cpp.md)     
            * **file** [**STM32Board.h**](STM32Board_8h.md) _Shared STM32F103 hardware initialisation for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _avionics nodes._    
        * **dir** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md)     
            * **file** [**SimGateway.cpp**](SimGateway_8cpp.md)     
            * **file** [**SimGateway.h**](SimGateway_8h.md) _RP2040 USB HID gateway library for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) __[_**SimGateway**_](namespaceSimGateway.md) _board._    

