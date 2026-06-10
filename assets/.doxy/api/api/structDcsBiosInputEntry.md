

# Struct DcsBiosInputEntry



[**ClassList**](annotated.md) **>** [**DcsBiosInputEntry**](structDcsBiosInputEntry.md)





* `#include <A4EC_InputMap.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  const char \* | [**arg0**](#variable-arg0)  <br>_value=0 argument ("0", "DEC", position string)_  |
|  const char \* | [**arg0fast**](#variable-arg0fast)  <br>_ACCEL\_ENCODER: fast decrement arg; nullptr for all other types._  |
|  const char \* | [**arg1**](#variable-arg1)  <br>_value=1 argument ("1", "INC"); nullptr for ACTION/MULTIPOS_  |
|  const char \* | [**arg1fast**](#variable-arg1fast)  <br>_ACCEL\_ENCODER: fast increment arg; nullptr for all other types._  |
|  uint16\_t | [**cmdId**](#variable-cmdid)  <br>_DCSIN\_\* compact ID — matches_ [_**A4EC\_CmdIds.h**_](A4EC__CmdIds_8h.md) _constant._ |
|  const char \* | [**name**](#variable-name)  <br>_DCS-BIOS control name for sendDcsBiosMessage()_  |
|  uint8\_t | [**type**](#variable-type)  <br>_One of_ [_**InputType**_](namespaceInputType.md) _::\* — determines_[_**PanelBridge**_](namespacePanelBridge.md) _dispatch._ |












































## Public Attributes Documentation




### variable arg0 

_value=0 argument ("0", "DEC", position string)_ 
```C++
const char* DcsBiosInputEntry::arg0;
```




<hr>



### variable arg0fast 

_ACCEL\_ENCODER: fast decrement arg; nullptr for all other types._ 
```C++
const char* DcsBiosInputEntry::arg0fast;
```




<hr>



### variable arg1 

_value=1 argument ("1", "INC"); nullptr for ACTION/MULTIPOS_ 
```C++
const char* DcsBiosInputEntry::arg1;
```




<hr>



### variable arg1fast 

_ACCEL\_ENCODER: fast increment arg; nullptr for all other types._ 
```C++
const char* DcsBiosInputEntry::arg1fast;
```




<hr>



### variable cmdId 

_DCSIN\_\* compact ID — matches_ [_**A4EC\_CmdIds.h**_](A4EC__CmdIds_8h.md) _constant._
```C++
uint16_t DcsBiosInputEntry::cmdId;
```




<hr>



### variable name 

_DCS-BIOS control name for sendDcsBiosMessage()_ 
```C++
const char* DcsBiosInputEntry::name;
```




<hr>



### variable type 

_One of_ [_**InputType**_](namespaceInputType.md) _::\* — determines_[_**PanelBridge**_](namespacePanelBridge.md) _dispatch._
```C++
uint8_t DcsBiosInputEntry::type;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/A4EC/A4EC_InputMap.h`

