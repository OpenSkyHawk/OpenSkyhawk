

# File LinkRecovery.h



[**FileList**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**LinkRecovery.h**](LinkRecovery_8h.md)

[Go to the source code of this file](LinkRecovery_8h_source.md)

_USB self-recovery ladder for_ [_**SimGateway**_](namespaceSimGateway.md) _— pure decision logic, no hardware._[More...](#detailed-description)

* `#include <stdint.h>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**OpenSkyhawk**](namespaceOpenSkyhawk.md) <br>_Thin wrapper over Adafruit\_ADS1115; see_ [_**ADS1115.h**_](ADS1115_8h.md) _._ |


## Classes

| Type | Name |
| ---: | :--- |
| struct | [**LinkRecovery**](structOpenSkyhawk_1_1LinkRecovery.md) <br>_Ladder state. Zero-initialised = healthy, no backoff._  |


















































## Detailed Description


The [**SimGateway**](namespaceSimGateway.md) is powered only from its USB-C, so while it runs it is plugged into something. Running but not configured by a host is therefore abnormal, and the failure this exists for (#355) looks exactly like that: the device dropped off the bus while the core kept running, and only a power cycle brought it back. The ladder escalates:



* unhealthy for LINK\_BAD\_MS → detach, then attach LINK\_REATTACH\_GAP\_MS later (toggles the D+ pull-up: the host sees a fresh plug-in)
* still unhealthy LINK\_REBOOT\_AFTER\_MS → reboot (fully resets the USB controller)
* after a recovery reboot → re-attach every LINK\_BACKOFF\_MS, never reboot again until the link has been healthy once




Step 3 keeps a [**SimGateway**](namespaceSimGateway.md) on a charger, or back-fed with no host, from reboot-looping.


Pure: takes `now` and the sampled health, returns what to do. [**SimGateway.cpp**](SimGateway_8cpp.md) applies the action, so SIMGATEWAY\_TEST builds drive the whole ladder with injected inputs.




**Copyright:**

GPL-2.0-only — see Firmware/LICENSE 





    

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/SimGateway/LinkRecovery.h`

