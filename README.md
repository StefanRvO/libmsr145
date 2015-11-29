# libmsr145
Library for communication with the MSR145 datalogger.

The library was/is created by reverse engineering the proprietary Windows driver.

The work was done using a MSR145B with firmware V5.18.

I have no idea if the software works for any other of the MSR dataloggers, or for different firmware versions.


This is very much a work in progress, and the API will NOT be stable for the foreseable future.

###Working:
#####General:
* Setting baudrate(If the baudrate is set to more than 38400 b/s, the device hangs if too many commands is sent in a row, also the baudrate is reset to 9600 b/s if a command have not been send in ~5 seconds).
* Calculation of 8-bit CRC checksum used by the protocol
* Formatting the memory
#####Reading:
* Read name
* Read Serial number
* Read current time of device
* Read recording start/stop settings
* Read recording start/stop time
* Read limit settings
* Getting live sensor data
* Read samples from recording (not 100% working when reading an active record)
* List recordings on device
* Reader "Marker" settings
* Read timer and sampling settings
* Read LED blink settings
#####Writing:
* Set name
* Set limits
* Set "Marker" settings
* Setting timer intervals, which sensors should be sample when the timer triggers, and if the blue LED should blink on timer trigger.
* Set time on device
* Stopping recording
* Starting recording (and settings for start and stop condition, e.g button, time, etc.)

###Not Working:
* Most things about calibration
* Reading MSR type and firmware version
* Everything not mentioned under "Working"
