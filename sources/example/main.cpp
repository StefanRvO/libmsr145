#include "libmsr145.hpp"
#include <iostream>
#include <bitset>
int main()
{
    MSRDevice msr("/dev/ttyUSB0");
    if(msr.isRecording()) msr.stopRecording();
    usleep(10000);
    //msr.format_memory();
    /*char *time_str = new char[500];
    auto t = msr.getTime();
    strftime(time_str, 500, "%D - %T", &t);
    std::cout << time_str << std::endl;*/
    //msr.setName("Osten");
    //std::cout << msr.getName() << std::endl;
    /*uint8_t command1[] = {0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t response_len = 8;
    uint8_t *response = new uint8_t[response_len];
    //while(1)
    //{
        msr.sendcommand(command1, sizeof(command1), response, response_len);
        for(int i = 0; i < 8; i++)
            printf("%02X ", response[i]);
        std::cout << std::endl;
    //}
    std::cout << std::endl;*/
    auto recordings = msr.getRecordinglist();
    /*char *time_str = new char[500];
    std::cout << recordings.size() << std::endl;
    int k = 0;
    for(auto i : recordings)
    {
        strftime(time_str, 500, "%D - %T", &(i.time));
        printf("%d\t%04X  %s   %u\n", k++, i.address, time_str, i.lenght);
    }*/
    //msr.set_baud(230400);
    std::cout << recordings.size() << std::endl;
    std::vector<sample> samples;
    //for(uint8_t i = 0; i< recordings.size(); i++)
    //{

    if(recordings.size())
    {
        samples = msr.getSamples(recordings[0]);
        for(size_t j = 0; j < samples.size(); j++)
        {
            //if(samples[j].type == sampletype::humidity)
            {
                printf("%08X\t %02X\t %d\t %f\n", samples[j].rawsample, (int)samples[j].type, samples[j].value, samples[j].timestamp / (float)(512));
            }
        }
    }
        //printf("%u\n\n\n", i);
    //}*/
    /*msr.set_baud(230400);
    auto rawdata = msr.getRawRecording(recordings[0]);
    std::cout << rawdata.size();
    for(size_t i = 0; i < rawdata.size();)
    {
        for(uint8_t j = 0; j < 4 && i < rawdata.size(); j++, i++)
        {
            //if(rawdata[i - j + 7] == 0x10)
            //std::cout << std::bitset<8> (rawdata[i]) << " ";
            printf("%02X ", rawdata[i]);
        }
        //if( rawdata[ i - 1] == 0x10)
        printf("\n" );
    }*/
    //std::cout << data.size() << std::endl;
    //uint8_t command[] = {0x85, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00};

    //printf("%02X\n", msr.calcChecksum(command, sizeof(command)));
    /*int16_t * returnvals = new int16_t[3];

    while(1)
    {
        msr.updateSensors();
        msr.getSensorData(returnvals, sampletype::pressure, sampletype::humidity, sampletype::T_pressure);
        for(uint8_t i = 0; i < 3; i++) printf("%d\n", returnvals[i]);
    }*/
    /*uint8_t *r = new uint8_t[8];
    uint8_t command[] =     {0x88, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t command2[] =    {0x88, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
    {
        msr.sendcommand(command2, sizeof(command2), r, 8);
        msr.sendcommand(command, sizeof(command), r, 8);
        for(uint8_t i = 0; i < 8; i++) printf("%02X ", r[i]);
        printf("\n");
    }*/

    //msr.set_baud(9600);
    //
    usleep(5000000);
    msr.reset_limits();
    msr.setTime();
    //for(uint8_t i = 0; i < 8; i++)
        msr.set_timer_interval((timer)1, 511);
        msr.set_timer_interval((timer)2, 512);

    for(uint8_t i = 0; i < 8; i++)
        msr.set_timer_measurements((timer)i, 0, 0);
    msr.set_timer_measurements((timer)1, active_measurement::humidity, 1);
    msr.set_timer_measurements((timer)2, active_measurement::pressure, 1);

    //msr.set_limit(sampletype::T_humidity, 0, 0, limit_setting::no_limit, limit_setting::rec_start_more_limit1_stop_less_limit2);
    //msr.set_limit(sampletype::humidity, 0, 0, limit_setting::no_limit, limit_setting::alarm_more_limit1_and_less_limit2);
    //msr.set_limit(sampletype::pressure, 0, 0, limit_setting::no_limit, limit_setting::alarm_more_limit1_and_less_limit2);
    msr.set_marker_settings(false, false);
    msr.start_recording( startcondition::now, nullptr, nullptr, false);
    //usleep(10000);
    /*char *time_str2 = new char[500];
    auto t = msr.getEndTime();
    strftime(time_str2, 500, "%D - %T", &t);
    std::cout << time_str2 << std::endl;*/
}
