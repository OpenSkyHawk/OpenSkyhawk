
# Class Hierarchy

This inheritance list is sorted roughly, but not completely, alphabetically:


* **class** [**OpenSkyhawk::InputBase**](classOpenSkyhawk_1_1InputBase.md) _Base class for all hardware-polled input objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._    
    * **class** [**OpenSkyhawk::Switch2Pos**](classOpenSkyhawk_1_1Switch2Pos.md) _Debounced 2-position GPIO switch — sends a ControlPacket CAN event on change._ 
* **class** [**OpenSkyhawk::OutputBase**](classOpenSkyhawk_1_1OutputBase.md) _Base class for all DCS-driven output objects on a_ [_**PanelGroup**_](namespacePanelGroup.md) _board._    
    * **class** [**OpenSkyhawk::IntegerOutput**](classOpenSkyhawk_1_1IntegerOutput.md) _Call an arbitrary function with the raw value from a ControlPacket._ 
    * **class** [**OpenSkyhawk::LED**](classOpenSkyhawk_1_1LED.md) _Drive a GPIO pin from a single bit of a DCS-BIOS output value._ 
* **struct** [**BatchState**](structBatchState.md) 
* **struct** [**DcsBiosInputEntry**](structDcsBiosInputEntry.md) 
* **struct** [**RxQueueEntry**](structRxQueueEntry.md) 
* **struct** [**TxQueueEntry**](structTxQueueEntry.md) 

