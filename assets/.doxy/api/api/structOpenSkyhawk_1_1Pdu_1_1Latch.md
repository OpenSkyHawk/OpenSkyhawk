

# Struct OpenSkyhawk::Pdu::Latch



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) **>** [**Latch**](structOpenSkyhawk_1_1Pdu_1_1Latch.md)



_A condition that must hold_ `setMs` _to raise and its clear condition hold_`clearMs` _to drop._

* `#include <RailMonitor.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**on**](#variable-on)   = `false`<br> |
|  bool | [**pending**](#variable-pending)   = `false`<br> |
|  uint32\_t | [**since**](#variable-since)   = `0`<br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**reset**](#function-reset) () <br> |
|  void | [**update**](#function-update) (bool cond, bool clear, uint32\_t setMs, uint32\_t clearMs, uint32\_t nowMs) <br>_Advance the latch._ `cond` _raises it,_`clear` _(evaluated while on) drops it._ |




























## Public Attributes Documentation




### variable on 

```C++
bool OpenSkyhawk::Pdu::Latch::on;
```




<hr>



### variable pending 

```C++
bool OpenSkyhawk::Pdu::Latch::pending;
```




<hr>



### variable since 

```C++
uint32_t OpenSkyhawk::Pdu::Latch::since;
```




<hr>
## Public Functions Documentation




### function reset 

```C++
inline void OpenSkyhawk::Pdu::Latch::reset () 
```




<hr>



### function update 

_Advance the latch._ `cond` _raises it,_`clear` _(evaluated while on) drops it._
```C++
void OpenSkyhawk::Pdu::Latch::update (
    bool cond,
    bool clear,
    uint32_t setMs,
    uint32_t clearMs,
    uint32_t nowMs
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/RailMonitor.h`

