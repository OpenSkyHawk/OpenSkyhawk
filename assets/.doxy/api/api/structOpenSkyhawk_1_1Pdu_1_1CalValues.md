

# Struct OpenSkyhawk::Pdu::CalValues



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) **>** [**CalValues**](structOpenSkyhawk_1_1Pdu_1_1CalValues.md)



_Calibration values the conversions need. Defaults = uncalibrated._ 

* `#include <PduMath.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**iGain12**](#variable-igain12)   = `10000`<br>_12 V current gain trim, 10000 = ×1_  |
|  uint16\_t | [**iGain5**](#variable-igain5)   = `10000`<br>_5 V current gain trim_  |
|  int16\_t | [**ntcOffsetDC**](#variable-ntcoffsetdc)   = `0`<br>_NTC one-point offset, 0.1 °C._  |
|  uint16\_t | [**vGain12**](#variable-vgain12)   = `10000`<br>_12 V voltage gain trim_  |
|  uint16\_t | [**vGain5**](#variable-vgain5)   = `10000`<br>_5 V voltage gain trim_  |
|  uint16\_t | [**vrefintMv**](#variable-vrefintmv)   = `0`<br>_this chip's Vrefint in mV; 0 = not calibrated (use VDDA\_NOMINAL\_MV)_  |
|  int16\_t | [**zero12DmA**](#variable-zero12dma)   = `0`<br>_12 V current zero offset, 0.1 mA_  |
|  int16\_t | [**zero5DmA**](#variable-zero5dma)   = `0`<br>_5 V current zero offset, 0.1 mA_  |












































## Public Attributes Documentation




### variable iGain12 

_12 V current gain trim, 10000 = ×1_ 
```C++
uint16_t OpenSkyhawk::Pdu::CalValues::iGain12;
```




<hr>



### variable iGain5 

_5 V current gain trim_ 
```C++
uint16_t OpenSkyhawk::Pdu::CalValues::iGain5;
```




<hr>



### variable ntcOffsetDC 

_NTC one-point offset, 0.1 °C._ 
```C++
int16_t OpenSkyhawk::Pdu::CalValues::ntcOffsetDC;
```




<hr>



### variable vGain12 

_12 V voltage gain trim_ 
```C++
uint16_t OpenSkyhawk::Pdu::CalValues::vGain12;
```




<hr>



### variable vGain5 

_5 V voltage gain trim_ 
```C++
uint16_t OpenSkyhawk::Pdu::CalValues::vGain5;
```




<hr>



### variable vrefintMv 

_this chip's Vrefint in mV; 0 = not calibrated (use VDDA\_NOMINAL\_MV)_ 
```C++
uint16_t OpenSkyhawk::Pdu::CalValues::vrefintMv;
```




<hr>



### variable zero12DmA 

_12 V current zero offset, 0.1 mA_ 
```C++
int16_t OpenSkyhawk::Pdu::CalValues::zero12DmA;
```




<hr>



### variable zero5DmA 

_5 V current zero offset, 0.1 mA_ 
```C++
int16_t OpenSkyhawk::Pdu::CalValues::zero5DmA;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/PduMath.h`

