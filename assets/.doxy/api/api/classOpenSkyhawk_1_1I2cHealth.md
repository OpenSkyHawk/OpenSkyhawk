

# Class OpenSkyhawk::I2cHealth



[**ClassList**](annotated.md) **>** [**OpenSkyhawk**](namespaceOpenSkyhawk.md) **>** [**I2cHealth**](classOpenSkyhawk_1_1I2cHealth.md)



_Per-device I2C circuit breaker. Mix into any class that talks to an I2C device._ [More...](#detailed-description)

* `#include <I2cHealth.h>`





Inherited by the following classes: [OpenSkyhawk::DrumDisplay](classOpenSkyhawk_1_1DrumDisplay.md)
































## Public Functions

| Type | Name |
| ---: | :--- |
|  bool | [**i2cHealthy**](#function-i2chealthy) () const<br>_Breaker state — true while the device last probed reachable._  |










## Protected Static Attributes

| Type | Name |
| ---: | :--- |
|  constexpr uint32\_t | [**I2C\_RETRY\_MS**](#variable-i2c_retry_ms)   = `2000`<br>_Back-off between retries once tripped (ms). A couple of seconds keeps the bus quiet._  |














## Protected Functions

| Type | Name |
| ---: | :--- |
| virtual bool | [**i2cProbe**](#function-i2cprobe) () = 0<br>_Contract: probe this device's reachability (e.g. the mux ACKs_ _and_ _the device ACKs)._ |
|  bool | [**i2cReachable**](#function-i2creachable) () <br>_Gate for every I2C op. Rate-limits the probe while tripped; trips/heals on the result._  |
|   | [**~I2cHealth**](#function-i2chealth) () = default<br> |




## Detailed Description


Usage: `class Foo : public  OutputBase , public I2cHealth { bool i2cProbe() override {...} };` then `if (!i2cReachable()) return;` before any I2C transaction. 


    
## Public Functions Documentation




### function i2cHealthy 

_Breaker state — true while the device last probed reachable._ 
```C++
inline bool OpenSkyhawk::I2cHealth::i2cHealthy () const
```




<hr>
## Protected Static Attributes Documentation




### variable I2C\_RETRY\_MS 

_Back-off between retries once tripped (ms). A couple of seconds keeps the bus quiet._ 
```C++
constexpr uint32_t OpenSkyhawk::I2cHealth::I2C_RETRY_MS;
```




<hr>
## Protected Functions Documentation




### function i2cProbe 

_Contract: probe this device's reachability (e.g. the mux ACKs_ _and_ _the device ACKs)._
```C++
virtual bool OpenSkyhawk::I2cHealth::i2cProbe () = 0
```





**Returns:**

true if reachable. The implementer records any fault detail it wants to report. 




**Note:**

Must be cheap — a single address probe, no payload — and must not throw or block beyond one bounded I2C transaction. 





        

<hr>



### function i2cReachable 

_Gate for every I2C op. Rate-limits the probe while tripped; trips/heals on the result._ 
```C++
inline bool OpenSkyhawk::I2cHealth::i2cReachable () 
```





**Returns:**

true → safe to talk to the device; false → skip the op (dead/absent, backing off). 





        

<hr>



### function ~I2cHealth 

```C++
OpenSkyhawk::I2cHealth::~I2cHealth () = default
```




<hr>

------------------------------
The documentation for this class was generated from the following file `Firmware/Libraries/PanelGroup/Helpers/I2cHealth/I2cHealth.h`

