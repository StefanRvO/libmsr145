#pragma once

namespace active_calibrations
{
    enum active_calibrations
    {
        humidity =          1 << 0,
        temperature_T =     1 << 1,
        temperature_RH =    1 << 2,
    };
}

namespace calibration_type
{
    enum calibration_type
    {
        temperature = 0x07,
        humidity    = 0x05,
    };
}

namespace active_measurement
{
    enum active_measurement
    {
        pressure = 1 << 0,
        humidity = 1 << 2,
        T1       = 1 << 3,
        bat      = 1 << 4,
        blink    = 1 << 16,
    };
}


enum startcondition
{
    now,
    push_start,
    push_start_stop,
    time_start,
    time_start_stop,
    time_stop,
};

enum limit_setting
{
    no_limit = 0x00,
    rec_less_limit2 = 0x01,
    rec_more_limit2 = 0x02,
    rec_more_limit1_and_less_limit2 = 0x03,
    rec_less_limit1_or_more_limit2 = 0x04,
    rec_start_more_limit1_stop_less_limit2 = 0x05,
    rec_start_less_limit1_stop_more_limit2 = 0x06,
    alarm_less_limit1 = 0x01 << 3,
    alarm_more_limit1 = 0x02 << 3,
    alarm_more_limit1_and_less_limit2 = 0x03 << 3,
    alarm_less_limit1_or_more_limit2 = 0x04 << 3,
};

enum sampletype
{
    pressure = 0x0,
    T_pressure = 0x1,
    humidity = 0x5,
    T_humidity = 0x6,
    bat = 0xE,
    ext1 = 0xA,
    ext2 = 0xB,
    ext3 = 0xC,
    ext4 = 0x7,
    timestamp = 0xF,
    unknown1 = 0x0D, //may be marker??
    end = 0xFFFF,
    none = 0x55, //Just a placeholder for none for use in getSensorData
};
