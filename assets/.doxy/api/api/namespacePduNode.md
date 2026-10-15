

# Namespace PduNode



[**Namespace List**](namespaces.md) **>** [**PduNode**](namespacePduNode.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**loop**](#function-loop) () <br>_Run one iteration: CAN, node status, sampling, telemetry, DiagSerial. Call every_ [_**loop()**_](namespacePduNode.md#function-loop) _._ |
|  const [**RailMonitor**](classOpenSkyhawk_1_1RailMonitor.md) & | [**rail12**](#function-rail12) () <br>_The 12 V rail monitor (bring-up / tests)._  |
|  const [**RailMonitor**](classOpenSkyhawk_1_1RailMonitor.md) & | [**rail5**](#function-rail5) () <br>_The 5 V rail monitor._  |
|  uint16\_t | [**scanRateHz**](#function-scanratehz) () <br>_Rail scans completed in the last full second — bring-up check of the 1 kHz target._  |
|  void | [**setup**](#function-setup) () <br>_Start the board, CAN and calibration, then send READY\_n. Call from_ [_**setup()**_](namespacePduNode.md#function-setup) _._ |
|  const [**BoardThermal**](classOpenSkyhawk_1_1BoardThermal.md) & | [**thermal**](#function-thermal) () <br>_The board thermal monitor._  |




























## Public Functions Documentation




### function loop 

_Run one iteration: CAN, node status, sampling, telemetry, DiagSerial. Call every_ [_**loop()**_](namespacePduNode.md#function-loop) _._
```C++
void PduNode::loop () 
```




<hr>



### function rail12 

_The 12 V rail monitor (bring-up / tests)._ 
```C++
const RailMonitor & PduNode::rail12 () 
```




<hr>



### function rail5 

_The 5 V rail monitor._ 
```C++
const RailMonitor & PduNode::rail5 () 
```




<hr>



### function scanRateHz 

_Rail scans completed in the last full second — bring-up check of the 1 kHz target._ 
```C++
uint16_t PduNode::scanRateHz () 
```




<hr>



### function setup 

_Start the board, CAN and calibration, then send READY\_n. Call from_ [_**setup()**_](namespacePduNode.md#function-setup) _._
```C++
void PduNode::setup () 
```




<hr>



### function thermal 

_The board thermal monitor._ 
```C++
const BoardThermal & PduNode::thermal () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduNode.cpp`

