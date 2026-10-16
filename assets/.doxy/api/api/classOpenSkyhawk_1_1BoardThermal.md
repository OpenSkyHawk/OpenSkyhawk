

# Class OpenSkyhawk::BoardThermal



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**BoardThermal**](classOpenSkyhawk_1_1BoardThermal.md)





* `#include <BoardThermal.h>`



Inherits the following classes: [OpenSkyhawk::FaultSource](classOpenSkyhawk_1_1FaultSource.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**BoardThermal**](#function-boardthermal) () = default<br> |
| virtual [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**faultCode**](#function-faultcode) () override const<br>_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._ |
| virtual const char \* | [**faultDetail**](#function-faultdetail) () override const<br>_Human-readable fault detail for the local DiagSerial tap only — never on the wire._  |
|  uint8\_t | [**flags**](#function-flags) () const<br>_PduStatusFlag bits this monitor owns: TEMP\_WARN, TEMP\_CRIT, NTC\_FAULT._  |
|  void | [**sample**](#function-sample) (int16\_t tempDC, bool ntcFault, bool armed) <br>_Feed one reading._  |
|  int16\_t | [**tempDC**](#function-tempdc) () const<br>_Latest temperature, 0.1 °C; NTC\_TEMP\_UNAVAILABLE when no reading yet / NTC fault._  |


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




### function BoardThermal 

```C++
OpenSkyhawk::BoardThermal::BoardThermal () = default
```




<hr>



### function faultCode 

_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._
```C++
virtual NodeFaultCode OpenSkyhawk::BoardThermal::faultCode () override const
```



Implements [*OpenSkyhawk::FaultSource::faultCode*](classOpenSkyhawk_1_1FaultSource.md#function-faultcode)


<hr>



### function faultDetail 

_Human-readable fault detail for the local DiagSerial tap only — never on the wire._ 
```C++
virtual const char * OpenSkyhawk::BoardThermal::faultDetail () override const
```



Implements [*OpenSkyhawk::FaultSource::faultDetail*](classOpenSkyhawk_1_1FaultSource.md#function-faultdetail)


<hr>



### function flags 

_PduStatusFlag bits this monitor owns: TEMP\_WARN, TEMP\_CRIT, NTC\_FAULT._ 
```C++
uint8_t OpenSkyhawk::BoardThermal::flags () const
```




<hr>



### function sample 

_Feed one reading._ 
```C++
void OpenSkyhawk::BoardThermal::sample (
    int16_t tempDC,
    bool ntcFault,
    bool armed
) 
```





**Parameters:**


* `tempDC` Board temperature, 0.1 °C (ignored when `ntcFault`). 
* `ntcFault` NTC channel open or shorted. 
* `armed` false during power-up blanking — the flags are held clear. 




        

<hr>



### function tempDC 

_Latest temperature, 0.1 °C; NTC\_TEMP\_UNAVAILABLE when no reading yet / NTC fault._ 
```C++
inline int16_t OpenSkyhawk::BoardThermal::tempDC () const
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/BoardThermal.h`

