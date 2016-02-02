#include "libmsr145.hpp"
#include <iostream>
#include <bitset>
int main()
{
    MSRDevice msr("/dev/ttyUSB0");

    /*auto recordings = msr.get_rec_list();
    char *time_str = new char[500];
    std::cout << recordings.size() << std::endl;
    int k = 0;
    for(auto i : recordings)
    {
        strftime(time_str, 500, "%D - %T", &(i.time));
        printf("%d\t%04X  %s   %u\n", k++, i.address, time_str, i.length);
    }
    //msr.set_baud(230400);
    std::cout << recordings.size() << std::endl;
    std::vector<sample> samples;
    //for(uint8_t i = 0; i< recordings.size(); i++)
    //{

    if(recordings.size())
    {
        samples = msr.get_samples(recordings[0]);
        for(size_t j = 0; j < samples.size(); j++)
        {
            //if(samples[j].type == sampletype::humidity)
            {
                printf("%08X\t %02X\t %d\t %f\n", samples[j].rawsample, (int)samples[j].type, samples[j].value, samples[j].timestamp / (float)(512));
            }
        }
    }*/

    /*msr.reset_limits();
    msr.set_time();
    for(uint8_t i = 0; i < 8; i++)
        msr.set_timer_interval(i, rand() % 400 + 10);

    for(uint8_t i = 0; i < 8; i++)
        msr.set_timer_measurements(i, active_measurement::humidity, 1);
*/
    //uint16_t data[4];
    //msr.setNamesAndCalibrationDate("BillyBob", "Gstar", 16, 5, 5);
    for(uint16_t i = 0; i < 0x100; i++)
        if(msr.set_calibrationdata((sampletype)i, 500, 600, 700, 800) == 0)
        {
            printf("%02X\n", i);
        }
    //msr.start_recording( startcondition::now, nullptr, nullptr, false);



}
