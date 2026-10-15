

# Class OpenSkyhawk::RailMonitor



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**RailMonitor**](classOpenSkyhawk_1_1RailMonitor.md)





* `#include <RailMonitor.h>`



Inherits the following classes: [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)












## Classes

| Type | Name |
| ---: | :--- |
| struct | [**Config**](structOpenSkyhawk_1_1RailMonitor_1_1Config.md) <br>_Per-rail limits._  |










































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**RailMonitor**](#function-railmonitor) (const [**Config**](structOpenSkyhawk_1_1RailMonitor_1_1Config.md) & cfg) <br> |
|  TelemPayload | [**buildPage**](#function-buildpage) (bool resetWindow) <br>_Build this rail's PDU\_RAIL\_\* page from the current window._  |
| virtual [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**faultCode**](#function-faultcode) () override const<br>_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._ |
| virtual const char \* | [**faultDetail**](#function-faultdetail) () override const<br>_Human-readable fault detail for the local DiagSerial tap only — never on the wire._  |
|  uint8\_t | [**flags**](#function-flags) () const<br>_Current PduRailFlag bits (all zero = healthy)._  |
|  int32\_t | [**lastMa**](#function-lastma) () const<br>_Last converted current, mA — DiagSerial / bring-up._  |
|  uint16\_t | [**lastMv**](#function-lastmv) () const<br>_Last converted voltage, mV — DiagSerial / bring-up._  |
|  uint16\_t | [**peakMa**](#function-peakma) () const<br>_Peak current since the last reset, mA (≥ 0)._  |
|  void | [**resetPeak**](#function-resetpeak) () <br>_Restart the peak — called when the PDU\_STATUS page is sent periodically._  |
|  void | [**sample**](#function-sample) (uint16\_t vMv, int32\_t iMa, bool vRailed, uint32\_t nowMs, bool armed) <br>_Feed one scan._  |


## Public Functions inherited from OpenSkyhawk::FaultSource

See [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)

| Type | Name |
| ---: | :--- |
| virtual [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**faultCode**](classOpenSkyhawk_1_1FaultSource.md#function-faultcode) () const<br>_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._ |
| virtual const char \* | [**faultDetail**](classOpenSkyhawk_1_1FaultSource.md#function-faultdetail) () const<br>_Human-readable fault detail for the local DiagSerial tap only — never on the wire._  |
|  [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) \* | [**next**](classOpenSkyhawk_1_1FaultSource.md#function-next) () const<br>_Next fault source; nullptr at end._  |




## Public Static Functions inherited from OpenSkyhawk::FaultSource

See [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)

| Type | Name |
| ---: | :--- |
|  [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) \* | [**head**](classOpenSkyhawk_1_1FaultSource.md#function-head) () <br>_Head of the self-registered fault-source list._  |












































## Protected Functions inherited from OpenSkyhawk::FaultSource

See [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)

| Type | Name |
| ---: | :--- |
|   | [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md#function-faultsource) () <br>_Registers this instance into the list._  |
|   | [**~FaultSource**](classOpenSkyhawk_1_1FaultSource.md#function-faultsource) () = default<br>_Protected, non-virtual: a base/mixin, never deleted through this type._  |






## Public Functions Documentation




### function RailMonitor 

```C++
explicit OpenSkyhawk::RailMonitor::RailMonitor (
    const Config & cfg
) 
```




<hr>



### function buildPage 

_Build this rail's PDU\_RAIL\_\* page from the current window._ 
```C++
TelemPayload OpenSkyhawk::RailMonitor::buildPage (
    bool resetWindow
) 
```





**Parameters:**


* `resetWindow` true for the periodic send (window restarts); false for an edge send. 




        

<hr>



### function faultCode 

_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._
```C++
virtual NodeFaultCode OpenSkyhawk::RailMonitor::faultCode () override const
```



Implements [*OpenSkyhawk::FaultSource::faultCode*](classOpenSkyhawk_1_1FaultSource.md#function-faultcode)


<hr>



### function faultDetail 

_Human-readable fault detail for the local DiagSerial tap only — never on the wire._ 
```C++
virtual const char * OpenSkyhawk::RailMonitor::faultDetail () override const
```



Implements [*OpenSkyhawk::FaultSource::faultDetail*](classOpenSkyhawk_1_1FaultSource.md#function-faultdetail)


<hr>



### function flags 

_Current PduRailFlag bits (all zero = healthy)._ 
```C++
uint8_t OpenSkyhawk::RailMonitor::flags () const
```




<hr>



### function lastMa 

_Last converted current, mA — DiagSerial / bring-up._ 
```C++
inline int32_t OpenSkyhawk::RailMonitor::lastMa () const
```




<hr>



### function lastMv 

_Last converted voltage, mV — DiagSerial / bring-up._ 
```C++
inline uint16_t OpenSkyhawk::RailMonitor::lastMv () const
```




<hr>



### function peakMa 

_Peak current since the last reset, mA (≥ 0)._ 
```C++
inline uint16_t OpenSkyhawk::RailMonitor::peakMa () const
```




<hr>



### function resetPeak 

_Restart the peak — called when the PDU\_STATUS page is sent periodically._ 
```C++
inline void OpenSkyhawk::RailMonitor::resetPeak () 
```




<hr>



### function sample 

_Feed one scan._ 
```C++
void OpenSkyhawk::RailMonitor::sample (
    uint16_t vMv,
    int32_t iMa,
    bool vRailed,
    uint32_t nowMs,
    bool armed
) 
```





**Parameters:**


* `vMv` Rail voltage, mV. 
* `iMa` Rail current, mA (signed — residual offset may read slightly negative). 
* `vRailed` Voltage channel at the top of the ADC range (can't measure). 
* `nowMs` millis(). 
* `armed` false during the power-up blanking window: statistics accumulate but every flag is held clear (rail ramp + inrush would otherwise trip UV / OC). 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/RailMonitor.h`

