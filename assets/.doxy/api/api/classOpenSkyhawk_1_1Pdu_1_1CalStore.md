

# Class OpenSkyhawk::Pdu::CalStore



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) **>** [**CalStore**](classOpenSkyhawk_1_1Pdu_1_1CalStore.md)



_The reserved flash page holding the calibration log._ 

* `#include <PduCalStore.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**CalStore**](#function-calstore) (uint32\_t pageAddr=0x0800FC00UL) <br> |
|  bool | [**erase**](#function-erase) () <br>_Erase the whole page — back to uncalibrated._  |
|  bool | [**load**](#function-load) (CalRecord & out) const<br>_Newest valid record into_ `out` _._ |
|  bool | [**save**](#function-save) (CalRecord & rec) <br>_Append_ `rec` _(sealed here, seq = previous + 1); erases the page first when full._ |
|  uint8\_t | [**usedSlots**](#function-usedslots) () const<br>_Slots holding a record (valid or not) — 0..CAL\_SLOTS._  |




























## Public Functions Documentation




### function CalStore 

```C++
inline explicit OpenSkyhawk::Pdu::CalStore::CalStore (
    uint32_t pageAddr=0x0800FC00UL
) 
```




<hr>



### function erase 

_Erase the whole page — back to uncalibrated._ 
```C++
bool OpenSkyhawk::Pdu::CalStore::erase () 
```




<hr>



### function load 

_Newest valid record into_ `out` _._
```C++
bool OpenSkyhawk::Pdu::CalStore::load (
    CalRecord & out
) const
```





**Returns:**

false (out = [**calDefaults()**](namespaceOpenSkyhawk_1_1Pdu.md#function-caldefaults)) when none. 





        

<hr>



### function save 

_Append_ `rec` _(sealed here, seq = previous + 1); erases the page first when full._
```C++
bool OpenSkyhawk::Pdu::CalStore::save (
    CalRecord & rec
) 
```





**Returns:**

true when the written slot reads back valid. 





        

<hr>



### function usedSlots 

_Slots holding a record (valid or not) — 0..CAL\_SLOTS._ 
```C++
uint8_t OpenSkyhawk::Pdu::CalStore::usedSlots () const
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduCalStore.h`

