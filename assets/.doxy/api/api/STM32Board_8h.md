

# File STM32Board.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**STM32Board**](dir_aa1816754c0645981f9c7af905857f7d.md) **>** [**STM32Board.h**](STM32Board_8h.md)

[Go to the source code of this file](STM32Board_8h_source.md)

_Shared STM32F103 hardware initialisation for_ [_**OpenSkyhawk**_](namespaceOpenSkyhawk.md) _avionics nodes._[More...](#detailed-description)

* `#include <Arduino.h>`
* `#include <stm32f1xx_hal_can.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**STM32Board**](namespaceSTM32Board.md) <br> |




















































## Detailed Description


Manages the bi-color status LED, DiagSerial, and CAN peripheral configuration — identical on every [**OpenSkyhawk**](namespaceOpenSkyhawk.md) STM32F103CBT6 board. All CAN bus operations (send, filter, start) go through [**CANProtocol**](namespaceCANProtocol.md).


Fixed hardware (same on every board — no constructor arguments):
* LED : PB14 (red) + PB15 (green), active HIGH
* UART : USART1 PA9 TX / PA10 RX @ 115200 (diagnostic tap)
* CAN : SN65HVD230 on PA11 (RX) / PA12 (TX)






**Version:**

0.2.0 




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/STM32Board/STM32Board.h`

