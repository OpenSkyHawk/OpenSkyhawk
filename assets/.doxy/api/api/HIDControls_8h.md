

# File HIDControls.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**HIDControls**](dir_8de7ffd664ed88ad14416481a318893f.md) **>** [**HIDControls.h**](HIDControls_8h.md)

[Go to the source code of this file](HIDControls_8h_source.md)

_CAN controlId constants for HID axes and buttons._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**CTRL\_BRAKE\_L**](HIDControls_8h.md#define-ctrl_brake_l)  `0x0014`<br> |
| define  | [**CTRL\_BRAKE\_R**](HIDControls_8h.md#define-ctrl_brake_r)  `0x0015`<br> |
| define  | [**CTRL\_ID\_HID\_MAX**](HIDControls_8h.md#define-ctrl_id_hid_max)  `0x009F`<br> |
| define  | [**CTRL\_ID\_HID\_MIN**](HIDControls_8h.md#define-ctrl_id_hid_min)  `0x0010`<br> |
| define  | [**CTRL\_PITCH**](HIDControls_8h.md#define-ctrl_pitch)  `0x0011`<br> |
| define  | [**CTRL\_ROLL**](HIDControls_8h.md#define-ctrl_roll)  `0x0010`<br> |
| define  | [**CTRL\_RUDDER**](HIDControls_8h.md#define-ctrl_rudder)  `0x0013`<br> |
| define  | [**CTRL\_THROTTLE**](HIDControls_8h.md#define-ctrl_throttle)  `0x0012`<br> |
| define  | [**CTRL\_TRIGGER**](HIDControls_8h.md#define-ctrl_trigger)  `0x0020`<br> |
| define  | [**CTRL\_ZOOM**](HIDControls_8h.md#define-ctrl_zoom)  `0x0016`<br> |

## Detailed Description


Shared between STM32 (via CANProtocol) and RP2040 ([**SimGateway**](namespaceSimGateway.md) sketches). Contains only #define constants — no classes, no functions, no state.


controlId routing by range: 0x0010–0x001F HID axes — routed to Joystick axis setters on [**SimGateway**](namespaceSimGateway.md) 0x0020–0x009F HID buttons — routed to Joystick button setters on [**SimGateway**](namespaceSimGateway.md)




**Version:**

0.1.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    
## Macro Definition Documentation





### define CTRL\_BRAKE\_L 

```C++
#define CTRL_BRAKE_L `0x0014`
```




<hr>



### define CTRL\_BRAKE\_R 

```C++
#define CTRL_BRAKE_R `0x0015`
```




<hr>



### define CTRL\_ID\_HID\_MAX 

```C++
#define CTRL_ID_HID_MAX `0x009F`
```




<hr>



### define CTRL\_ID\_HID\_MIN 

```C++
#define CTRL_ID_HID_MIN `0x0010`
```




<hr>



### define CTRL\_PITCH 

```C++
#define CTRL_PITCH `0x0011`
```




<hr>



### define CTRL\_ROLL 

```C++
#define CTRL_ROLL `0x0010`
```




<hr>



### define CTRL\_RUDDER 

```C++
#define CTRL_RUDDER `0x0013`
```




<hr>



### define CTRL\_THROTTLE 

```C++
#define CTRL_THROTTLE `0x0012`
```




<hr>



### define CTRL\_TRIGGER 

```C++
#define CTRL_TRIGGER `0x0020`
```




<hr>



### define CTRL\_ZOOM 

```C++
#define CTRL_ZOOM `0x0016`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/HIDControls/HIDControls.h`

