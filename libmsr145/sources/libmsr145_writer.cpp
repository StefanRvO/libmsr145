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

void MSR_Writer::insert_time_in_command(struct tm *timeset, uint8_t *command)
{
    command[2] = timeset->tm_min;
    command[3] = timeset->tm_hour + ((timeset->tm_sec & 0x7) << 5);
    command[4] = (timeset->tm_mday - 1) + ((timeset->tm_sec >> 3) << 5);
    command[5] = timeset->tm_mon;
    command[6] = timeset->tm_year - 100;
}

void MSR_Writer::set_time(struct tm *timeset/* need to be a mktime() formated, eg not have more than 60 seconds*/)
{
    //if nullptr, set to local time
    if(timeset == nullptr)
    {
        time_t rawtime;
        time(&rawtime);
        timeset = localtime(&rawtime);
    }
    uint8_t command[] = {0x8D, 0x00, 0x00 /*minutes*/, 0x00 /*hours*, 3 highest is lsb seconds*/,
        0x00/*days 3 highest is msb seconds*/, 0x00/* month*/, 0x00/*year*/};
    insert_time_in_command(timeset, command);

    this->send_command(command, sizeof(command), nullptr, 8);
}

void MSR_Writer::set_names_and_calibration_date(std::string deviceName, std::string calibrationName,
    uint8_t year, uint8_t month, uint8_t day)
{   //These three settings needs to be set together, as they corrupt eachother if they are not.
    //years start at 0 = 2000.
    //months start at 0 = january.
    //Day start at 0 = 1th.
    //if name is shorter than 12 characters (length of name), append spaces
    //This is also done by the proprietary driver
    while(deviceName.size() < 12)
        deviceName.push_back(' ');
    while(calibrationName.size() < 8)
        calibrationName.push_back(' ');

    //this is some kind of "setup command" without it, the device won't accept the next commands
    uint8_t command_1[] = {0x85, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00};

    //these commands sets 4 characters each
    uint8_t command_2[] = {0x84, 0x05, 0x00, (uint8_t)deviceName[0], (uint8_t)deviceName[1], (uint8_t)deviceName[2], (uint8_t)deviceName[3]};
    uint8_t command_3[] = {0x84, 0x05, 0x01, (uint8_t)deviceName[4], (uint8_t)deviceName[5], (uint8_t)deviceName[6], (uint8_t)deviceName[7]};
    uint8_t command_4[] = {0x84, 0x05, 0x02, (uint8_t)deviceName[8], (uint8_t)deviceName[9], (uint8_t)deviceName[10], (uint8_t)deviceName[11]};
    uint8_t command_5[] = {0x84, 0x05, 0x03, year, month, day, 0x05};
    uint8_t command_6[] = {0x84, 0x05, 0x04, (uint8_t)calibrationName[0], (uint8_t)calibrationName[1], (uint8_t)calibrationName[2], (uint8_t)calibrationName[3]};
    uint8_t command_7[] = {0x84, 0x05, 0x05, (uint8_t)calibrationName[4], (uint8_t)calibrationName[5], (uint8_t)calibrationName[6], (uint8_t)calibrationName[7]};

    this->send_command(command_1, sizeof(command_1), nullptr, 8);
    this->send_command(command_2, sizeof(command_2), nullptr, 8);
    this->send_command(command_3, sizeof(command_3), nullptr, 8);
    this->send_command(command_4, sizeof(command_4), nullptr, 8);
    this->send_command(command_5, sizeof(command_5), nullptr, 8);
    this->send_command(command_6, sizeof(command_6), nullptr, 8);
    this->send_command(command_7, sizeof(command_7), nullptr, 8);

}


void MSR_Writer::start_recording(startcondition start_set,
    struct tm *starttime, struct tm *stoptime, bool ringbuffer)
{
    //command for setting up when a recording should start
    uint8_t recording_setup[] = {0x84, 0x02, 0x01, 0x00, (uint8_t)(!ringbuffer), 0x00, 0x00};
    if(starttime != nullptr)
    {
        uint8_t set_start_time[] = {0x8D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
        insert_time_in_command(starttime, set_start_time);
        this->send_command(set_start_time, sizeof(set_start_time), nullptr, 8);
    }
    if(stoptime != nullptr)
    {
        uint8_t set_stop_time[] = {0x8D, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
        insert_time_in_command(stoptime, set_stop_time);
        this->send_command(set_stop_time, sizeof(set_stop_time), nullptr, 8);
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
        break;

    }
    this->send_command(recording_setup, sizeof(recording_setup), nullptr, 8);

    //start the recording
    uint8_t recording_start[] = {0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->send_command(recording_start, sizeof(recording_start), nullptr, 8);
}


void MSR_Writer::set_timer_interval(timer t, uint64_t interval) //interval is in 1/512 seconds
{
    uint8_t set_command[] = {0x84, 0x01, (uint8_t)t, 0x00, 0x00, 0x00, 0x00};
    set_command[3] = interval & 0xFF;
    set_command[4] = (interval >> 8) & 0xFF;
    set_command[5] = (interval >> 16) & 0xFF;
    set_command[6] = (interval >> 24) & 0xFF;
    this->send_command(set_command, sizeof(set_command), nullptr, 8);
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
    this->send_command(settings, sizeof(settings), nullptr, 8);
}


void MSR_Writer::set_limit(sampletype type, uint16_t limit1, uint16_t limit2,
    limit_setting record_limit, limit_setting alarm_limit)
{
    uint8_t type_byte = type;
    uint8_t limit_setting_byte = record_limit | alarm_limit;
    uint8_t set_limit1[] = {0x89, 0x0A, type_byte, limit_setting_byte, 0x00, (uint8_t)(limit1 & 0xFF), (uint8_t)(limit1 >> 8)};
    uint8_t set_limit2[] = {0x89, 0x0B, type_byte, 0x00, 0x00, (uint8_t)(limit2 & 0xFF), (uint8_t)(limit2  >> 8)};
    this->send_command(set_limit1, sizeof(set_limit1), nullptr, 8);
    this->send_command(set_limit2, sizeof(set_limit2), nullptr, 8);
}

void MSR_Writer::reset_limits()
{   //this resets all limits to their disabled state
    uint8_t reset_cmd[] = {0x89, 0x09, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    this->send_command(reset_cmd, sizeof(reset_cmd), nullptr, 8);
}

void MSR_Writer::set_marker_settings(bool marker_on, bool alarm_confirm_on)
{
    uint8_t set_cmd[] =  {0x89, 0x08, (uint8_t)marker_on, (uint8_t)alarm_confirm_on, 0x00, 0x00, 0x00};
    this->send_command(set_cmd, sizeof(set_cmd), nullptr, 8);
}

void MSR_Writer::set_calibrationdata(sampletype type, uint16_t point_1_target, uint16_t point_1_actual,
    uint16_t point_2_target, uint16_t point_2_actual)
{   //may need to be set before each recording (not throughly investigated yet.)
    uint8_t getpoint1[] = {0x89, 0x0C, (uint8_t)type, (uint8_t)(point_1_target & 0xFF), (uint8_t)(point_1_target >> 8),
        (uint8_t)(point_1_actual & 0xFF), (uint8_t)(point_1_actual >> 8)};
    uint8_t getpoint2[] = {0x89, 0x0D, (uint8_t)type, (uint8_t)(point_2_target & 0xFF), (uint8_t)(point_2_target >> 8),
        (uint8_t)(point_2_actual & 0xFF), (uint8_t)(point_2_actual >> 8)};
    this->send_command(getpoint1, sizeof(getpoint1), nullptr, 8);
    this->send_command(getpoint2, sizeof(getpoint2), nullptr, 8);
}
