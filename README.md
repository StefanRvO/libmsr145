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
* Calculation of 8-bit CRC checksum used by the protocol
* Read samples from recording (not 100% working when reading an active record)
* Setting blink interval of blue LED
* Stopping recording
* Starting recording (and settings for start and stop condition, e.g button, time, etc.)
* Getting live sensor data
#####Not Working:
* Everything else
