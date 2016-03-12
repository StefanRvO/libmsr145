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
    /*for(uint16_t i = 0; i < 0x100; i++)
        if(msr.set_calibrationdata((active_calibrations::active_calibrations)i, 500, 600, 700, 800) == 0)
        {
            printf("%02X\n", i);
        }
    */
    //msr.start_recording( startcondition::now, nullptr, nullptr, false);
    //msr.set_time();

    uint16_t point_1_target = 500, point_1_actual = 600, point_2_target = 700, point_2_actual = 800;
    uint16_t point_1_target_read, point_1_actual_read, point_2_target_read, point_2_actual_read;

    for(int i = 0;  i <= 0xFF; i++)
    {
        printf("%d\n", i);
        msr.set_calibrationdata(calibration_type::calibration_type(i), point_1_target, point_1_actual,
                        point_2_target, point_2_actual);
        msr.get_calibrationdata(calibration_type::calibration_type(0x05), &point_1_target_read, &point_1_actual_read, &point_2_target_read, &point_2_actual_read);
        printf("%d\t%d\t%d\t%d\t%d\n", 0x05, point_1_target_read, point_1_actual_read, point_2_target_read, point_2_actual_read);
        msr.get_calibrationdata(calibration_type::calibration_type(0x07), &point_1_target_read, &point_1_actual_read, &point_2_target_read, &point_2_actual_read);
        printf("%d\t%d\t%d\t%d\t%d\n", 0x07, point_1_target_read, point_1_actual_read, point_2_target_read, point_2_actual_read);

    }




}
