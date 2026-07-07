

# Class OpenSkyhawk::FaultSource



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md)



_A source of node faults — implemented by any object that can fault (#163)._ [More...](#detailed-description)

* `#include <NodeStatus.h>`





Inherited by the following classes: [OpenSkyhawk::DrumDisplay](classOpenSkyhawk_1_1DrumDisplay.md)
































## Public Functions

| Type | Name |
| ---: | :--- |
| virtual [**NodeFaultCode**](NodeStatus_8h.md#enum-nodefaultcode) | [**faultCode**](#function-faultcode) () const<br>_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._ |
| virtual const char \* | [**faultDetail**](#function-faultdetail) () const<br>_Human-readable fault detail for the local DiagSerial tap only — never on the wire._  |
|  [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) \* | [**next**](#function-next) () const<br>_Next fault source; nullptr at end._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) \* | [**head**](#function-head) () <br>_Head of the self-registered fault-source list._  |






















## Protected Functions

| Type | Name |
| ---: | :--- |
|   | [**FaultSource**](#function-faultsource) () <br>_Registers this instance into the list._  |
|   | [**~FaultSource**](#function-faultsource) () = default<br>_Protected, non-virtual: a base/mixin, never deleted through this type._  |




## Detailed Description


Self-registers into a static list at construction; a node-level aggregator walks `FaultSource::head()` to decide node health, pick the primary `faultCode()` for HEALTH\_n, and print `faultDetail()` to DiagSerial. Implementers report _cached_ state only — cheap, const, no blocking I/O — since the aggregator runs on the periodic health path. Examples: [**DrumDisplay**](classOpenSkyhawk_1_1DrumDisplay.md) (I2C), a PDU rail monitor (over/under-voltage/short), a [**PanelBridge**](namespacePanelBridge.md) host-link watchdog.




**Note:**

**Lifetime: a [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) must have static/global lifetime** (same rule as [**OutputBase**](classOpenSkyhawk_1_1OutputBase.md) / [**InputBase**](classOpenSkyhawk_1_1InputBase.md)). Registration is permanent — there is no unregister — so a stack/local [**FaultSource**](classOpenSkyhawk_1_1FaultSource.md) would leave a dangling pointer in the registry the aggregator walks. Construct fault sources as globals (or members of a global object), never on the stack. 





    
## Public Functions Documentation




### function faultCode 

_Current fault code (_ NodeFaultCode::NONE _when healthy). Cheap/const, cached state only._
```C++
inline virtual NodeFaultCode OpenSkyhawk::FaultSource::faultCode () const
```




<hr>



### function faultDetail 

_Human-readable fault detail for the local DiagSerial tap only — never on the wire._ 
```C++
inline virtual const char * OpenSkyhawk::FaultSource::faultDetail () const
```




<hr>



### function next 

_Next fault source; nullptr at end._ 
```C++
FaultSource * OpenSkyhawk::FaultSource::next () const
```




<hr>
## Public Static Functions Documentation




### function head 

_Head of the self-registered fault-source list._ 
```C++
static FaultSource * OpenSkyhawk::FaultSource::head () 
```




<hr>
## Protected Functions Documentation




### function FaultSource 

_Registers this instance into the list._ 
```C++
OpenSkyhawk::FaultSource::FaultSource () 
```




<hr>



### function ~FaultSource 

_Protected, non-virtual: a base/mixin, never deleted through this type._ 
```C++
OpenSkyhawk::FaultSource::~FaultSource () = default
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/NodeStatus/NodeStatus.h`

