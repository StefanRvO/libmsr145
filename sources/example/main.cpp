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
    auto recordings = msr.getRecordings();
    char *time_str = new char[500];
    std::cout << recordings.size() << std::endl;
    for(auto i : recordings)
    {
        strftime(time_str, 500, "%D - %T", &(i.time));
        printf("%04X  %s   %u\n", i.address, time_str, i.lenght);
    }
    /*auto data = msr.readRecording(recordings[5]);
    for(auto i : data)
    {
        printf("%02X\n", i);
    }*/
    uint8_t command[] = {0x8B, 0x00, 0x00, 0x1C, 0x00, 0x20, 0x04};

    printf("%02X\n", msr.calcChecksum(command, sizeof(command)));
}
