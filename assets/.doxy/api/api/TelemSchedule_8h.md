

# File TelemSchedule.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PowerMonitor**](dir_8872fbf3a1c0d8f3f8e90f9314489a2d.md) **>** [**TelemSchedule.h**](TelemSchedule_8h.md)

[Go to the source code of this file](TelemSchedule_8h_source.md)

_When to send which TELEM\_n page — periodic rotation plus rate-limited edge sends (#216)._ [More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |
| namespace | [**Pdu**](namespaceOpenSkyhawk_1_1Pdu.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**TelemSchedule**](classOpenSkyhawk_1_1Pdu_1_1TelemSchedule.md) <br> |


















































## Detailed Description



* Periodic: one frame per TELEM\_PERIOD\_MS (the HEALTH\_n rate), rotating through the pages, so each page refreshes every PAGES × period. A periodic send is the one that restarts that page's statistics window.
* Edge: when a page's flag byte differs from what was last sent for it, send that page now — at most once per EDGE\_MIN\_MS per page; a change inside the hold-off goes out when it ends. An edge send does not restart the window. Every page starts from an all-clear baseline (0), so a condition present at boot goes out promptly instead of waiting for its periodic slot.




Hardware-free — the caller supplies the clock and the current flag bytes.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PowerMonitor/TelemSchedule.h`

