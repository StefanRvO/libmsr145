#include "libmsr145.hpp"
#include <iostream>
int main()
{
    MSRDevice msr("/dev/ttyUSB0");
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
    msr.set_baud230400();
    auto recordings = msr.getRecordinglist();
    char *time_str = new char[500];
    //std::cout << recordings.size() << std::endl;
    for(auto i : recordings)
    {
        strftime(time_str, 500, "%D - %T", &(i.time));
        printf("%04X  %s   %u\n", i.address, time_str, i.lenght);
    }
    std::vector<sample> samples;
    //for(uint8_t i = 0; i< recordings.size(); i++)
    //{
        samples = msr.getSamples(recordings[0]);

        for(size_t j = 0; j < samples.size(); j++)
        {
            if(samples[j].type == sampletype::temp)
            {
                printf("%06X\n", samples[j].value);
            }
        }
        //printf("%u\n\n\n", i);
    //}
    /*auto rawdata = msr.getRawRecording(recordings[0]);
    for(size_t i = 0; i < rawdata.size();)
    {
        for(uint8_t j = 0; j < 32 && i < rawdata.size(); j++, i++)
        {
            printf("%02X ", rawdata[i]);
        }
        printf("\n" );
    }*/
    //std::cout << data.size() << std::endl;
    //uint8_t command[] = {0x85, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00};

    //printf("%02X\n", msr.calcChecksum(command, sizeof(command)));
}
