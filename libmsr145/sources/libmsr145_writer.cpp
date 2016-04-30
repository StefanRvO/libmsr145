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
#include <iostream>

void MSR_Writer::insert_time_in_command(struct tm *timeset, uint8_t *command)
{
    command[2] = timeset->tm_min;
    command[3] = timeset->tm_hour + ((timeset->tm_sec & 0x7) << 5);
    command[4] = (timeset->tm_mday - 1) + ((timeset->tm_sec >> 3) << 5);
    command[5] = timeset->tm_mon;
    command[6] = timeset->tm_year - 100;
}

int MSR_Writer::set_time(struct tm *timeset/* need to be a mktime() formated, eg not have more than 60 seconds*/)
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
    return this->send_command(command, sizeof(command), nullptr, 8);
}

int MSR_Writer::set_names_and_calibration_date(std::string deviceName, std::string calibrationName,
    uint8_t year, uint8_t month, uint8_t day, uint8_t active_calib)
{   //These three settings needs to be set together, as they corrupt eachother if they are not.
    //also, we need to read the calibration points and save them again, as these are also corrupted

    //years start at 0 = 2000.
    //months start at 0 = january.
    //Day start at 0 = 1th.
    //if name is shorter than 12 characters (length of name), append spaces
    //This is also done by the proprietary driver

    //for saving the stored calibration points
    uint16_t calibpoints[2][5];
    calibpoints[0][0] = calibration_type::humidity;
    calibpoints[1][0] = calibration_type::temperature;
    for(int i = 0; i < 2; i++)
    {
        get_calibrationdata(calibration_type::calibration_type(calibpoints[i][0]), calibpoints[i] + 1, calibpoints[i] + 2, calibpoints[i] + 3, calibpoints[i] + 4);
    }
    float L1_offset, L1_gain;
    get_L1_offset_gain(&L1_offset, &L1_gain);
    std::string L1_unit = get_L1_unit_str();

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
    uint8_t command_5[] = {0x84, 0x05, 0x03, year, month, day, active_calib};
    uint8_t command_6[] = {0x84, 0x05, 0x04, (uint8_t)calibrationName[0], (uint8_t)calibrationName[1], (uint8_t)calibrationName[2], (uint8_t)calibrationName[3]};
    uint8_t command_7[] = {0x84, 0x05, 0x05, (uint8_t)calibrationName[4], (uint8_t)calibrationName[5], (uint8_t)calibrationName[6], (uint8_t)calibrationName[7]};
    int returnval = 0;
    returnval |= this->send_command(command_1, sizeof(command_1), nullptr, 8);
    returnval |= this->send_command(command_2, sizeof(command_2), nullptr, 8);
    returnval |= this->send_command(command_3, sizeof(command_3), nullptr, 8);
    returnval |= this->send_command(command_4, sizeof(command_4), nullptr, 8);
    returnval |= this->send_command(command_5, sizeof(command_5), nullptr, 8);
    returnval |= this->send_command(command_6, sizeof(command_6), nullptr, 8);
    returnval |= this->send_command(command_7, sizeof(command_7), nullptr, 8);

    //set the read calibrations
    set_L1_unit(L1_unit);
    set_L1_offset_gain(L1_offset, L1_gain);
    for(int i = 0; i < 2; i++)
    {
        set_calibrationdata(calibration_type::calibration_type(calibpoints[i][0]), calibpoints[i][1], calibpoints[i][2], calibpoints[i][3], calibpoints[i][4]);
    }

    return returnval;
}


int MSR_Writer::start_recording(startcondition start_set,
    struct tm *starttime, struct tm *stoptime, bool ringbuffer)
{
    int returnval = 0;
    //command for setting up when a recording should start
    uint8_t recording_setup[] = {0x84, 0x02, 0x01, 0x00, (uint8_t)(!ringbuffer), 0x00, 0x00};
    if(starttime != nullptr)
    {
        uint8_t set_start_time[] = {0x8D, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
        insert_time_in_command(starttime, set_start_time);
        returnval |= this->send_command(set_start_time, sizeof(set_start_time), nullptr, 8);
    }
    if(stoptime != nullptr)
    {
        uint8_t set_stop_time[] = {0x8D, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
        insert_time_in_command(stoptime, set_stop_time);
        returnval |= this->send_command(set_stop_time, sizeof(set_stop_time), nullptr, 8);
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
    returnval |= this->send_command(recording_setup, sizeof(recording_setup), nullptr, 8);

    //start the recording
    uint8_t recording_start[] = {0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    returnval |= this->send_command(recording_start, sizeof(recording_start), nullptr, 8);
    return returnval;
}


int MSR_Writer::set_timer_interval(uint8_t t, uint64_t interval) //interval is in 1/512 seconds
{
    uint8_t set_command[] = {0x84, 0x01, t, 0x00, 0x00, 0x00, 0x00};
    set_command[3] = interval & 0xFF;
    set_command[4] = (interval >> 8) & 0xFF;
    set_command[5] = (interval >> 16) & 0xFF;
    set_command[6] = (interval >> 24) & 0xFF;
    return this->send_command(set_command, sizeof(set_command), nullptr, 8);
}


int MSR_Writer::set_timer_measurements(uint8_t t, uint8_t bitmask, bool blink, bool active)
{   //printf("%02X", bitmask)
    uint8_t blinkbyte = blink << 7;
    uint8_t settings[] = {0x84, 0x00, t,
        0x01, //control if timer is on or off
        0x00,
        bitmask,
        blinkbyte};
        if(active == false) settings[3] = 0x00;
    return this->send_command(settings, sizeof(settings), nullptr, 8);
}


int MSR_Writer::set_limit(sampletype type, uint16_t limit1, uint16_t limit2,
    limit_setting record_limit, limit_setting alarm_limit)
{
    uint8_t type_byte = type;
    uint8_t limit_setting_byte = record_limit | alarm_limit;
    uint8_t set_limit1[] = {0x89, 0x0A, type_byte, limit_setting_byte, 0x00, (uint8_t)(limit1 & 0xFF), (uint8_t)(limit1 >> 8)};
    uint8_t set_limit2[] = {0x89, 0x0B, type_byte, 0x00, 0x00, (uint8_t)(limit2 & 0xFF), (uint8_t)(limit2  >> 8)};
    int returnval = 0;
    returnval |= this->send_command(set_limit1, sizeof(set_limit1), nullptr, 8);
    returnval |= this->send_command(set_limit2, sizeof(set_limit2), nullptr, 8);
    return returnval;
}

int MSR_Writer::reset_limits()
{   //this resets all limits to their disabled state
    uint8_t reset_cmd[] = {0x89, 0x09, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    return this->send_command(reset_cmd, sizeof(reset_cmd), nullptr, 8);
}

int MSR_Writer::set_marker_settings(bool marker_on, bool alarm_confirm_on)
{
    uint8_t set_cmd[] =  {0x89, 0x08, (uint8_t)marker_on, (uint8_t)alarm_confirm_on, 0x00, 0x00, 0x00};
    return this->send_command(set_cmd, sizeof(set_cmd), nullptr, 8);
}

int MSR_Writer::set_calibrationdata(calibration_type::calibration_type type, uint16_t point_1_target, uint16_t point_1_actual,
    uint16_t point_2_target, uint16_t point_2_actual)
{
    //std::cout << "S" << int(type) << " " << point_1_target << " " << point_1_actual << " "  << point_2_target << " "  << point_2_actual << std::endl;
    uint8_t setpoint1[] = {0x89, 0x0C, (uint8_t)type, (uint8_t)(point_1_target & 0xFF), (uint8_t)(point_1_target >> 8),
        (uint8_t)(point_1_actual & 0xFF), (uint8_t)(point_1_actual >> 8)};
    uint8_t setpoint2[] = {0x89, 0x0D, (uint8_t)type, (uint8_t)(point_2_target & 0xFF), (uint8_t)(point_2_target >> 8),
        (uint8_t)(point_2_actual & 0xFF), (uint8_t)(point_2_actual >> 8)};
        int returnval = 0;
    returnval |= this->send_command(setpoint1, sizeof(setpoint1), nullptr, 8);
    returnval |= this->send_command(setpoint2, sizeof(setpoint2), nullptr, 8);
    return returnval;
}

int MSR_Writer::set_L1_unit(std::string unit)
{
    while(unit.size() < 4) unit += " ";
    uint8_t cmd[] = {0x89, 0x03, 0x09, (uint8_t)unit[0], (uint8_t)unit[1], (uint8_t)unit[2], (uint8_t)unit[3]};
    return send_command(cmd, sizeof(cmd), nullptr, 8);
}

int MSR_Writer::set_L1_offset_gain(float offset, float gain)
{
    uint8_t *offset_ptr = (uint8_t *)&offset;
    uint8_t *gain_ptr = (uint8_t *)&gain;
    uint8_t cmd1[] = {0x89, 0x04, 0x09, offset_ptr[1], offset_ptr[2], offset_ptr[3], 0x00};
    uint8_t cmd2[] = {0x89, 0x05, 0x09, gain_ptr[1], gain_ptr[2], gain_ptr[3], 0x00};
    int returnval = 0;
    returnval |= this->send_command(cmd1, sizeof(cmd1), nullptr, 8);
    returnval |= this->send_command(cmd2, sizeof(cmd2), nullptr, 8);
    return returnval;
}
