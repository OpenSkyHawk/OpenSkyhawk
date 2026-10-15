

# Struct OpenSkyhawk::RailMonitor::Config



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**RailMonitor**](classOpenSkyhawk_1_1RailMonitor.md) **>** [**Config**](structOpenSkyhawk_1_1RailMonitor_1_1Config.md)



_Per-rail limits._ 

* `#include <RailMonitor.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**fuseOpenCheck**](#variable-fuseopencheck)  <br>_true on the 12 V rail; a blown 5 V fuse powers the node down_  |
|  const char \* | [**name**](#variable-name)  <br>_"12V" / "5V" — DiagSerial detail only_  |
|  uint16\_t | [**ocMa**](#variable-ocma)  <br>_over-current threshold_  |
|  uint16\_t | [**ovMv**](#variable-ovmv)  <br>_over-voltage threshold_  |
|  [**TelemPage**](NodeStatus_8h.md#enum-telempage) | [**page**](#variable-page)  <br>_PDU\_RAIL\_12V or PDU\_RAIL\_5V._  |
|  uint16\_t | [**uvMv**](#variable-uvmv)  <br>_under-voltage threshold_  |












































## Public Attributes Documentation




### variable fuseOpenCheck 

_true on the 12 V rail; a blown 5 V fuse powers the node down_ 
```C++
bool OpenSkyhawk::RailMonitor::Config::fuseOpenCheck;
```




<hr>



### variable name 

_"12V" / "5V" — DiagSerial detail only_ 
```C++
const char* OpenSkyhawk::RailMonitor::Config::name;
```




<hr>



### variable ocMa 

_over-current threshold_ 
```C++
uint16_t OpenSkyhawk::RailMonitor::Config::ocMa;
```




<hr>



### variable ovMv 

_over-voltage threshold_ 
```C++
uint16_t OpenSkyhawk::RailMonitor::Config::ovMv;
```




<hr>



### variable page 

_PDU\_RAIL\_12V or PDU\_RAIL\_5V._ 
```C++
TelemPage OpenSkyhawk::RailMonitor::Config::page;
```




<hr>



### variable uvMv 

_under-voltage threshold_ 
```C++
uint16_t OpenSkyhawk::RailMonitor::Config::uvMv;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/RailMonitor.h`

