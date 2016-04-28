#include "libmsr145.hpp"
#include <iostream>
#include <bitset>
int main()
{
    MSRDevice msr("/dev/ttyUSB0");

    std::vector<sampletype> sensor_to_poll;
    for(int i = 0; i <= 0xF; i++)
    {
        sensor_to_poll.push_back((sampletype)i);
    }
    msr.update_sensors();
    auto sensor_readings = msr.get_sensor_data(sensor_to_poll);
    msr.update_sensors();
    sensor_readings = msr.get_sensor_data(sensor_to_poll);
    for(uint8_t i = 0; i < sensor_readings.size(); i++)
        std::cout << (int)sensor_to_poll[i] << "\t" << sensor_readings[i] << std::endl;





}
