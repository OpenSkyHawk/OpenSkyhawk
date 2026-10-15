

# Class OpenSkyhawk::Pdu::TelemSchedule



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) **>** [**TelemSchedule**](classOpenSkyhawk_1_1Pdu_1_1TelemSchedule.md)





* `#include <TelemSchedule.h>`























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**EDGE\_MIN\_MS**](#variable-edge_min_ms)   = `100`<br>_per-page edge-send hold-off_  |
|  constexpr uint8\_t | [**PAGES**](#variable-pages)   = `3`<br>_PDU\_RAIL\_12V, PDU\_RAIL\_5V, PDU\_STATUS._  |
|  constexpr uint32\_t | [**TELEM\_PERIOD\_MS**](#variable-telem_period_ms)   = `1000`<br>_one frame per second per node_  |














## Public Functions

| Type | Name |
| ---: | :--- |
|  int8\_t | [**next**](#function-next) (uint32\_t nowMs, const uint8\_t flags, bool & periodic) <br>_Which page to send now, if any._  |
|  void | [**sent**](#function-sent) (uint8\_t slot, uint8\_t flags, uint32\_t nowMs) <br>_Record that page_ `slot` _went out carrying_`flags` _at_`nowMs` _._ |
|  void | [**start**](#function-start) (uint32\_t nowMs) <br>_Start the clock; the first periodic frame is due one period later._  |




























## Public Static Attributes Documentation




### variable EDGE\_MIN\_MS 

_per-page edge-send hold-off_ 
```C++
constexpr uint32_t OpenSkyhawk::Pdu::TelemSchedule::EDGE_MIN_MS;
```




<hr>



### variable PAGES 

_PDU\_RAIL\_12V, PDU\_RAIL\_5V, PDU\_STATUS._ 
```C++
constexpr uint8_t OpenSkyhawk::Pdu::TelemSchedule::PAGES;
```




<hr>



### variable TELEM\_PERIOD\_MS 

_one frame per second per node_ 
```C++
constexpr uint32_t OpenSkyhawk::Pdu::TelemSchedule::TELEM_PERIOD_MS;
```




<hr>
## Public Functions Documentation




### function next 

_Which page to send now, if any._ 
```C++
int8_t OpenSkyhawk::Pdu::TelemSchedule::next (
    uint32_t nowMs,
    const uint8_t flags,
    bool & periodic
) 
```





**Parameters:**


* `nowMs` millis(). 
* `flags` Current flag byte of each page (index = page slot). 
* `periodic` Out: true for the periodic (window-restarting) send. 



**Returns:**

Page slot 0..PAGES-1, or -1 when nothing is due. 





        

<hr>



### function sent 

_Record that page_ `slot` _went out carrying_`flags` _at_`nowMs` _._
```C++
void OpenSkyhawk::Pdu::TelemSchedule::sent (
    uint8_t slot,
    uint8_t flags,
    uint32_t nowMs
) 
```




<hr>



### function start 

_Start the clock; the first periodic frame is due one period later._ 
```C++
void OpenSkyhawk::Pdu::TelemSchedule::start (
    uint32_t nowMs
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/TelemSchedule.h`

