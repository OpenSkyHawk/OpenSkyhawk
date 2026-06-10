

# Namespace OpenSkyhawk



[**Namespace List**](namespaces.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md)



_Output and input classes for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _panel boards._[More...](#detailed-description)
















## Classes

| Type | Name |
| ---: | :--- |
| class | [**InputBase**](classOpenSkyhawk_1_1InputBase.md) <br>_Base class for all hardware-polled input objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._ |
| class | [**IntegerOutput**](classOpenSkyhawk_1_1IntegerOutput.md) <br>_Call an arbitrary function with the raw value from a ControlPacket._  |
| class | [**LED**](classOpenSkyhawk_1_1LED.md) <br>_Drive a GPIO pin from a single bit of a DCS-BIOS output value._  |
| class | [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) <br>_Base class for all DCS-driven output objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._ |
| class | [**Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) <br>_Debounced 2-position GPIO switch — sends a ControlPacket CAN event on change._  |


















































## Detailed Description


Objects are declared at global scope in a sketch; constructors self-register into static linked lists. [**PanelGroup::loop()**](namespacePanelGroup.md#function-loop) dispatches incoming ControlPacket CAN frames to all registered [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) objects and polls all registered [**InputBase**](classOpenSkyhawk_1_1InputBase.md) objects every iteration. 


    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/PanelGroup.h`

