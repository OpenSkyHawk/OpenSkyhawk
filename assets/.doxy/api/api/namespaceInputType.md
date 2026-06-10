

# Namespace InputType



[**Namespace List**](namespaces.md) **>** [**InputType**](namespaceInputType.md)




























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint8\_t | [**ACCEL\_ENCODER**](#variable-accel_encoder)   = `3`<br>_RotaryAcceleratedEncoder — 0–3._  |
|  constexpr uint8\_t | [**ACTION**](#variable-action)   = `1`<br>_ActionButton — sends 1 (press only)_  |
|  constexpr uint8\_t | [**ANALOG**](#variable-analog)   = `5`<br>_AnalogInput — sends raw 0–65535 ADC reading._  |
|  constexpr uint8\_t | [**ENCODER**](#variable-encoder)   = `2`<br>_RotaryEncoder — sends 0=CCW, 1=CW._  |
|  constexpr uint8\_t | [**MULTIPOS**](#variable-multipos)   = `4`<br>_SwitchMultiPos / RotarySwitch / AnalogMultiPos._  |
|  constexpr uint8\_t | [**SWITCH**](#variable-switch)   = `0`<br>_Switch2Pos — sends 0 or 1._  |










































## Public Static Attributes Documentation




### variable ACCEL\_ENCODER 

_RotaryAcceleratedEncoder — 0–3._ 
```C++
constexpr uint8_t InputType::ACCEL_ENCODER;
```




<hr>



### variable ACTION 

_ActionButton — sends 1 (press only)_ 
```C++
constexpr uint8_t InputType::ACTION;
```




<hr>



### variable ANALOG 

_AnalogInput — sends raw 0–65535 ADC reading._ 
```C++
constexpr uint8_t InputType::ANALOG;
```




<hr>



### variable ENCODER 

_RotaryEncoder — sends 0=CCW, 1=CW._ 
```C++
constexpr uint8_t InputType::ENCODER;
```




<hr>



### variable MULTIPOS 

_SwitchMultiPos / RotarySwitch / AnalogMultiPos._ 
```C++
constexpr uint8_t InputType::MULTIPOS;
```




<hr>



### variable SWITCH 

_Switch2Pos — sends 0 or 1._ 
```C++
constexpr uint8_t InputType::SWITCH;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/A4EC/A4EC_InputMap.h`

