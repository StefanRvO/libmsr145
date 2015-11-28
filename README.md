# libmsr145
Library for communication with the MSR145 datalogger.

The library was/is created by reverse engineering the proprietary Windows driver.

The work was done using a MSR145B with firmware V5.18.

I have no idea if the software works for any other of the MSR dataloggers, or for different firmware versions.


This is very much a work in progress, and very few things are working yet.

#####Working:

* Read name
* Set name
* Read Serial number
* List recordings on device
* Read current time of device
* Set time on device
* Setting baudrate(If the baudrate is set to more than 38400 b/s, the device hangs if too many commands is sent in a row, also the baudrate is reset to 9600 b/s if a command have not been send in ~5 seconds).

* Calculation of 8-bit CRC checksum used by the protocol
* Read samples from recording (not 100% working when reading an active record)
* Stopping recording
* Starting recording (and settings for start and stop condition, e.g button, time, etc.)
* Getting live sensor data
* Setting timer intervals, which sensors should be sample when the timer triggers, and if the blue LED should blink on timer trigger.
* Formatting the memory

#####Not Working:
* Everything else
