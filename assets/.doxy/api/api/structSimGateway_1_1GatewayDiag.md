

# Struct SimGateway::GatewayDiag



[**ClassList**](annotated.md) **>** [**GatewayDiag**](structSimGateway_1_1GatewayDiag.md)



_Link-health counters and event times (#355). Reported to the host as_ `DIAG` _._[More...](#detailed-description)






















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**cdcStalls**](#variable-cdcstalls)  <br>_CDC stall onsets: DTR held, bytes pending, no drain for CDC\_STALL\_MS._  |
|  uint32\_t | [**firstUartErrMs**](#variable-firstuarterrms)  <br>_First PL011 error this boot._  |
|  uint32\_t | [**firstUsbLossMs**](#variable-firstusblossms)  <br>_First healthy → unhealthy host-link edge._  |
|  uint16\_t | [**framesDropped**](#variable-framesdropped)  <br>_Calibration frames dropped for lack of CDC room._  |
|  uint32\_t | [**lastUartErrMs**](#variable-lastuarterrms)  <br>_Latest PL011 error._  |
|  uint32\_t | [**lastUsbLossMs**](#variable-lastusblossms)  <br>_Latest such edge._  |
|  uint16\_t | [**linesCorrupt**](#variable-linescorrupt)  <br>_Lines dropped for a non-ASCII byte._  |
|  uint16\_t | [**linesDropped**](#variable-linesdropped)  <br>_Lines dropped for lack of CDC room._  |
|  uint16\_t | [**linesOverlong**](#variable-linesoverlong)  <br>_Lines dropped for exceeding RELAY\_LINE\_MAX._  |
|  uint8\_t | [**recoveryReboots**](#variable-recoveryreboots)  <br>_Recovery-ladder reboots. Survives a warm reboot._  |
|  uint8\_t | [**resetReason**](#variable-resetreason)  <br>[_**OpenSkyhawk::CalDiagResetReason**_](namespaceOpenSkyhawk.md#enum-caldiagresetreason) _for this boot._ |
|  uint16\_t | [**uartBreak**](#variable-uartbreak)  <br>_Status ticks with RSR BE set._  |
|  uint16\_t | [**uartFraming**](#variable-uartframing)  <br>_Status ticks with RSR FE set._  |
|  uint16\_t | [**uartOverrun**](#variable-uartoverrun)  <br>_Status ticks with RSR OE set._  |
|  uint16\_t | [**uartParity**](#variable-uartparity)  <br>_Status ticks with RSR PE set._  |
|  uint16\_t | [**uartRestarts**](#variable-uartrestarts)  <br>_FAULT self-clears (UART restarted)._  |
|  uint16\_t | [**uartRxOverflow**](#variable-uartrxoverflow)  <br>_Status ticks on which the software RX queue had overflowed._  |
|  uint16\_t | [**usbReattaches**](#variable-usbreattaches)  <br>_Recovery-ladder detach/attach cycles._  |
|  uint8\_t | [**watchdogResets**](#variable-watchdogresets)  <br>_Hardware watchdog resets. Survives a warm reboot._  |












































## Detailed Description


Times are millis() since this boot, 0 = never. Counters saturate at 0xFFFF. The field meanings are the `CAL_DIAG_*` offsets in [**AxisCal.h**](AxisCal_8h.md), which define the wire layout. 


    
## Public Attributes Documentation




### variable cdcStalls 

_CDC stall onsets: DTR held, bytes pending, no drain for CDC\_STALL\_MS._ 
```C++
uint16_t SimGateway::GatewayDiag::cdcStalls;
```




<hr>



### variable firstUartErrMs 

_First PL011 error this boot._ 
```C++
uint32_t SimGateway::GatewayDiag::firstUartErrMs;
```




<hr>



### variable firstUsbLossMs 

_First healthy → unhealthy host-link edge._ 
```C++
uint32_t SimGateway::GatewayDiag::firstUsbLossMs;
```




<hr>



### variable framesDropped 

_Calibration frames dropped for lack of CDC room._ 
```C++
uint16_t SimGateway::GatewayDiag::framesDropped;
```




<hr>



### variable lastUartErrMs 

_Latest PL011 error._ 
```C++
uint32_t SimGateway::GatewayDiag::lastUartErrMs;
```




<hr>



### variable lastUsbLossMs 

_Latest such edge._ 
```C++
uint32_t SimGateway::GatewayDiag::lastUsbLossMs;
```




<hr>



### variable linesCorrupt 

_Lines dropped for a non-ASCII byte._ 
```C++
uint16_t SimGateway::GatewayDiag::linesCorrupt;
```




<hr>



### variable linesDropped 

_Lines dropped for lack of CDC room._ 
```C++
uint16_t SimGateway::GatewayDiag::linesDropped;
```




<hr>



### variable linesOverlong 

_Lines dropped for exceeding RELAY\_LINE\_MAX._ 
```C++
uint16_t SimGateway::GatewayDiag::linesOverlong;
```




<hr>



### variable recoveryReboots 

_Recovery-ladder reboots. Survives a warm reboot._ 
```C++
uint8_t SimGateway::GatewayDiag::recoveryReboots;
```




<hr>



### variable resetReason 

[_**OpenSkyhawk::CalDiagResetReason**_](namespaceOpenSkyhawk.md#enum-caldiagresetreason) _for this boot._
```C++
uint8_t SimGateway::GatewayDiag::resetReason;
```




<hr>



### variable uartBreak 

_Status ticks with RSR BE set._ 
```C++
uint16_t SimGateway::GatewayDiag::uartBreak;
```




<hr>



### variable uartFraming 

_Status ticks with RSR FE set._ 
```C++
uint16_t SimGateway::GatewayDiag::uartFraming;
```




<hr>



### variable uartOverrun 

_Status ticks with RSR OE set._ 
```C++
uint16_t SimGateway::GatewayDiag::uartOverrun;
```




<hr>



### variable uartParity 

_Status ticks with RSR PE set._ 
```C++
uint16_t SimGateway::GatewayDiag::uartParity;
```




<hr>



### variable uartRestarts 

_FAULT self-clears (UART restarted)._ 
```C++
uint16_t SimGateway::GatewayDiag::uartRestarts;
```




<hr>



### variable uartRxOverflow 

_Status ticks on which the software RX queue had overflowed._ 
```C++
uint16_t SimGateway::GatewayDiag::uartRxOverflow;
```




<hr>



### variable usbReattaches 

_Recovery-ladder detach/attach cycles._ 
```C++
uint16_t SimGateway::GatewayDiag::usbReattaches;
```




<hr>



### variable watchdogResets 

_Hardware watchdog resets. Survives a warm reboot._ 
```C++
uint8_t SimGateway::GatewayDiag::watchdogResets;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/SimGateway.h`

