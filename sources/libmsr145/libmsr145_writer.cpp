/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "libmsr145.hpp"
#include <string>
#include <cstdio>

void MSR_Writer::setTime(struct tm *timeset/* need to be a mktime() formated, eg not have more than 60 seconds*/)
{
    //if nullptr, set to local time
    if(timeset == nullptr)
    {
        time_t rawtime;
        time(&rawtime);
        timeset = localtime(&rawtime);
    }
    uint8_t command[] = {0x8D, 0x00,
        0x00 /*minutes*/,
        0x00 /*hours*, 3 highest is lsb seconds*/,
        0x00/*days 3 highest is msb seconds*/,
        0x00/* month*/,
        0x00/*year*/};
    command[2] = timeset->tm_min;
    command[3] = timeset->tm_hour + ((timeset->tm_sec & 0x7) << 5);
    command[4] = (timeset->tm_mday - 1) + ((timeset->tm_sec >> 3) << 5);
    command[5] = timeset->tm_mon;
    command[6] = timeset->tm_year - 100;

    this->sendcommand(command, sizeof(command), nullptr, 8);
}



void MSR_Writer::setName(std::string name)
{
    //if name is shorter than 12 characters (length of name), append spaces
    //This is also done by the proprietary driver
    while(name.size() < 12)
        name.push_back(' ');

    //this is some kind of "setup command" without it, the device won't accept the next commands
    uint8_t command_1[] = {0x85, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00};

    //these commands sets 4 characters each
    uint8_t command_2[] = {0x84, 0x05, 0x00, (uint8_t)name[0], (uint8_t)name[1], (uint8_t)name[2], (uint8_t)name[3]};
    uint8_t command_3[] = {0x84, 0x05, 0x01, (uint8_t)name[4], (uint8_t)name[5], (uint8_t)name[6], (uint8_t)name[7]};
    uint8_t command_4[] = {0x84, 0x05, 0x02, (uint8_t)name[8], (uint8_t)name[9], (uint8_t)name[10], (uint8_t)name[11]};

    this->sendcommand(command_1, sizeof(command_1), nullptr, 8);
    this->sendcommand(command_2, sizeof(command_2), nullptr, 8);
    this->sendcommand(command_3, sizeof(command_3), nullptr, 8);
    this->sendcommand(command_4, sizeof(command_4), nullptr, 8);
}

void MSR_Writer::stopRecording()
{
    uint8_t stop_command[] = {0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->sendcommand(stop_command, sizeof(stop_command), nullptr, 8);
}

void MSR_Writer::start_recording(startcondition start_set,
    struct tm *starttime, struct tm *stoptime, bool ringbuffer)
{
    //command for setting up when a recording should start
    uint8_t recording_setup[] = {0x84, 0x02, 0x01, 0x00, (uint8_t)(!ringbuffer), 0x00, 0x00};
    if(starttime != nullptr)
    {
        uint8_t set_start_time[] = {0x8D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
        set_start_time[2] = starttime->tm_min;
        set_start_time[3] = starttime->tm_hour + ((starttime->tm_sec & 0x7) << 5);
        set_start_time[4] = (starttime->tm_mday - 1) + ((starttime->tm_sec >> 3) << 5);
        set_start_time[5] = starttime->tm_mon;
        set_start_time[6] = starttime->tm_year - 100;
        this->sendcommand(set_start_time, sizeof(set_start_time), nullptr, 8);
    }
    if(stoptime != nullptr)
    {
        uint8_t set_stop_time[] = {0x8D, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
        set_stop_time[2] = stoptime->tm_min;
        set_stop_time[3] = stoptime->tm_hour + ((stoptime->tm_sec & 0x7) << 5);
        set_stop_time[4] = (stoptime->tm_mday - 1) + ((stoptime->tm_sec >> 3) << 5);
        set_stop_time[5] = stoptime->tm_mon;
        set_stop_time[6] = stoptime->tm_year - 100;
        this->sendcommand(set_stop_time, sizeof(set_stop_time), nullptr, 8);
    }
    switch(start_set)
    {
        case startcondition::now:
            recording_setup[2] = 0x01;
            break;
        case startcondition::push_start:
            recording_setup[2] = 0x03;
            break;
        case startcondition::push_start_stop:
            recording_setup[2] = 0x03;
            recording_setup[3] = 0x03;
            break;
        case startcondition::time_start:
            recording_setup[2] = 0x02;
            break;
        case startcondition::time_start_stop:
            recording_setup[2] = 0x02;
            recording_setup[3] = 0x02;
            break;
        case startcondition::time_stop:
            recording_setup[2] = 0x01;
            recording_setup[3] = 0x02;

    }
    this->sendcommand(recording_setup, sizeof(recording_setup), nullptr, 8);

    //start the recording
    uint8_t recording_start[] = {0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->sendcommand(recording_start, sizeof(recording_start), nullptr, 8);
}


void MSR_Writer::set_timer_interval(timer t, uint64_t interval) //interval is in 1/512 seconds
{
    uint8_t set_command[] = {0x84, 0x01, (uint8_t)t, 0x00, 0x00, 0x00, 0x00};
    set_command[3] = interval & 0xFF;
    set_command[4] = (interval >> 8) & 0xFF;
    set_command[5] = (interval >> 16) & 0xFF;
    set_command[6] = (interval >> 24) & 0xFF;
    this->sendcommand(set_command, sizeof(set_command), nullptr, 8);
}


void MSR_Writer::set_timer_measurements(timer t, uint8_t bitmask, bool blink)
{   //printf("%02X", bitmask)
    uint8_t blinkbyte = blink << 7;
    uint8_t settings[] = {0x84, 0x00, (uint8_t)t,
        0x01, //control if timer is on or off
        0x00,
        bitmask,
        blinkbyte};
        if(bitmask == 0x00 && blinkbyte == false) settings[3] = 0x00;
    this->sendcommand(settings, sizeof(settings), nullptr, 8);
}
void MSR_Writer::format_memory()
{
    stopRecording(); //stop current recording before format.
    uint16_t start_address = 0x0000;
    uint16_t end_address = 0x03FF;
    uint8_t erase_cmd[] = {0x8A, 0x06, 0x00,
        0x00 /*adress lsb*/, 0x00 /*address msb*/,
        0x5A, 0xA5};
    uint8_t start_cmd1[] = {0x8A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->sendcommand(start_cmd1, sizeof(start_cmd1), nullptr, 8);

    uint8_t confirm_command[] = {0x8A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *returnval = new uint8_t[8];
    for(uint16_t addr = start_address; addr <= end_address; addr++)
    {
        erase_cmd[3] = addr & 0xFF;
        erase_cmd[4] = addr >> 8;
        this->sendcommand(erase_cmd, sizeof(erase_cmd), returnval, 8);
        do
        {
            this->sendcommand(confirm_command, sizeof(confirm_command), returnval, 8);
        } while(returnval[1] != 0xBC);
    }
    delete[] returnval;
}

void MSR_Writer::set_limit(sampletype type, uint16_t limit1, uint16_t limit2,
    limit_setting record_limit, limit_setting alarm_limit)
{
    uint8_t type_byte = type;
    uint8_t limit_setting_byte = record_limit | alarm_limit;
    uint8_t set_limit1[] = {0x89, 0x0A, type_byte, limit_setting_byte, 0x00, (uint8_t)(limit1 & 0xFF), (uint8_t)(limit1 >> 8)};
    uint8_t set_limit2[] = {0x89, 0x0B, type_byte, 0x00, 0x00, (uint8_t)(limit2 & 0xFF), (uint8_t)(limit2  >> 8)};
    this->sendcommand(set_limit1, sizeof(set_limit1), nullptr, 8);
    this->sendcommand(set_limit2, sizeof(set_limit2), nullptr, 8);
}

void MSR_Writer::reset_limits()
{   //this resets all limits to their disabled state
    uint8_t reset_cmd[] = {0x89, 0x09, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    this->sendcommand(reset_cmd, sizeof(reset_cmd), nullptr, 8);
}

void MSR_Writer::set_marker_settings(bool marker_on, bool alarm_confirm_on)
{
    uint8_t set_cmd[] =  {0x89, 0x08, (uint8_t)marker_on, (uint8_t)alarm_confirm_on, 0x00, 0x00, 0x00};
    this->sendcommand(set_cmd, sizeof(set_cmd), nullptr, 8);
}
