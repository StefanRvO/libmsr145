#include "msr145_tool.hpp"
#include <ctime>
#include <iostream>
#include <sstream>
static const char *timeformat = "%Y:%m:%d--%H:%M:%S";


void MSRTool::print_status()
{
    update_sensors();
    std::cout << "Device Status:" << std::endl;
    std::cout << "Serial Number: " << get_serial() << std::endl;
    std::cout << "Device Name:" << get_name() << std::endl;
    std::cout << "Device time: " << get_device_time_str() << std::endl;
    std::cout << "Device is recording: " << is_recording() << std::endl;
    bool marker_on, alarm_confirm_on;
    get_marker_setting(&marker_on, &alarm_confirm_on);
    std::cout << "Marker: " << marker_on << std::endl;
    std::cout << "Alarm confirm: " <<  alarm_confirm_on << std::endl;
    std::cout << "Sampling intervals:" << std::endl;
    std::cout << get_interval_string() << std::endl;
    std::cout << "Current sensor measurements:" << std::endl;
    std::vector<sampletype> sensor_to_poll;
    sensor_to_poll.push_back(sampletype::pressure);
    sensor_to_poll.push_back(sampletype::T_pressure);
    sensor_to_poll.push_back(sampletype::humidity);
    sensor_to_poll.push_back(sampletype::T_humidity);
    sensor_to_poll.push_back(sampletype::bat);
    auto sensor_readings = get_sensor_data(sensor_to_poll);
    for(uint8_t i = 0; i < sensor_readings.size(); i++)
        std::cout << get_sensor_string(sensor_to_poll[i], sensor_readings[i]);
    std::cout << get_start_settings_str() << std::endl;
    std::cout << "Limit settings:" << std::endl;
    std::cout << get_limits_str() << std::endl;
}

std::string MSRTool::get_limits_str()
{
    std::stringstream ret_str;
    uint16_t activated_limits = get_general_lim_settings();
    for(uint8_t i = 0; i < 0xF; i++) //run through all the types
    {
        if(!(activated_limits & 1 << i)) continue;
        switch(i)
        {
            case sampletype::pressure: case sampletype::T_pressure:
            case sampletype::humidity: case sampletype::T_humidity:
            case sampletype::bat:
            ret_str << get_sample_limit_str((sampletype)i);
                break;
            default:
                std::cout << "unknown limit type: " << (int)i << std::endl;
        }
    }
    if(ret_str.str().size())
        return ret_str.str();
    else
        return std::string("\tNone");
}

std::string MSRTool::get_sample_limit_str(sampletype type)
{
    std::stringstream ret_str;
    ret_str << "\tLimits for ";
    switch(type)
    {
        case pressure:
            ret_str << "Pressure (mbar): ";
            break;
        case T_pressure:
            ret_str << "Temperature(p) (C): ";
            break;
        case humidity:
            ret_str << "Humidity (%): ";
            break;
        case T_humidity:
            ret_str << "Temperature(RH) (C): ";
            break;
        case bat:
            ret_str << "Battery (V): ";
            break;
        default:
            assert(false); //this should not happen
    }
    uint8_t rec_settings, alarm_settings;
    uint16_t limit1, limit2;
    get_sample_lim_setting(type, &rec_settings, &alarm_settings, &limit1, &limit2);
    ret_str << "L1=" << convert_to_unit(type, limit1) << ", L2="
        << convert_to_unit(type, limit2) << std::endl;
    switch(rec_settings)
    {
        case rec_less_limit2:
            ret_str << "\t\tRecord Limit: S<L2" << std::endl;
            break;
        case rec_more_limit2:
            ret_str << "\t\tRecord Limit: S>L2" << std::endl;
            break;
        case rec_more_limit1_and_less_limit2:
            ret_str << "\t\tRecord Limit: L1<S<L2" << std::endl;
            break;
        case rec_less_limit1_or_more_limit2:
            ret_str << "\t\tRecord Limit: S<L1 or S>L2" << std::endl;
            break;
        case rec_start_more_limit1_stop_less_limit2:
            ret_str << "\t\tRecord Limit: Start: S>L1, Stop: S<L2" << std::endl;
            break;
        case rec_start_less_limit1_stop_more_limit2:
            ret_str << "\t\tRecord Limit: Start: S<L1, Stop: S>L2" << std::endl;
            break;
    }
    switch(alarm_settings)
    {
        case alarm_less_limit1:
            ret_str << "\t\tAlarm Limit: S<L1" << std::endl;
            break;
        case alarm_more_limit1:
            ret_str << "\t\tAlarm Limit: S>L1" << std::endl;
            break;
        case alarm_more_limit1_and_less_limit2:
            ret_str << "\t\tAlarm Limit: L1<S<L2" << std::endl;
            break;
        case alarm_less_limit1_or_more_limit2:
            ret_str << "\t\tAlarm Limit: S<L1 or S>L2" << std::endl;
            break;
    }
    return ret_str.str();
}

std::string MSRTool::get_start_settings_str()
{
    std::stringstream ret_str;
    bool bufferon;
    startcondition startcon;
    get_start_setting(&bufferon, &startcon);
    ret_str << "Settings for start:\n";
    ret_str << "\tRingbuffer on: " << bufferon << std::endl;
    ret_str << "\tStartsetting:\t";
    switch(startcon)
    {
        case startcondition::now:
            ret_str << "Start imidediately" << std::endl;
            break;
        case startcondition::push_start:
            ret_str << "Start on button push" << std::endl;
            break;
        case startcondition::push_start_stop:
            ret_str << "Start and stop on button push" << std::endl;
            break;
        case startcondition::time_start:
            ret_str << "Start on\t" << get_start_time_str() << std::endl;
            break;
        case startcondition::time_stop:
            ret_str << "Start imidediately" << std::endl;
            ret_str << "\t\t\tStop on\t\t" << get_end_time_str() << std::endl;
            break;
        case startcondition::time_start_stop:
            ret_str << "Start on\t" << get_start_time_str() << std::endl;
            ret_str << "\t\t\tStop on\t\t" << get_end_time_str() << std::endl;
            break;
    }
    return ret_str.str();
}

std::string MSRTool::get_sensor_string(sampletype type, uint16_t value)
{
    std::stringstream ret_str;
    switch(type)
    {
        case pressure:
            ret_str << "\tPressure (mbar):\t" << convert_to_unit(type, value) << std::endl;
            break;
        case T_pressure:
            ret_str << "\tTemperature(p) (C):\t" << convert_to_unit(type, value) << std::endl;
            break;
        case humidity:
            ret_str << "\tHumidity (%):\t\t" << convert_to_unit(type, value) << std::endl;
            break;
        case T_humidity:
            ret_str << "\tTemperature(RH) (C):\t" << convert_to_unit(type, value) << std::endl;
            break;
        case bat:
            ret_str << "\tBattery (V):\t\t" << convert_to_unit(type, value) << std::endl;
            break;
        default:
            assert(false); //this should not happen
    }
    return ret_str.str();
}

float MSRTool::convert_to_unit(sampletype type, uint16_t value)
{
    switch(type)
    {
        case pressure:
            //returns in unit of mbar
            return value/10.;
        case T_pressure:
            //return in ⁰C
            return value/10.;
        case humidity:
            //return in %
            return value/100.;
        case T_humidity:
            //return in ⁰C
            return value/100.;
        case bat:
            //return in unit of volts
            return value; //not figured out yet
        default:
            assert(false); //this should not happen
    }
}

std::string MSRTool::get_interval_string()
{
    std::stringstream return_str;
    return_str.precision(3);
    return_str.setf( std::ios::fixed, std:: ios::floatfield );
    std::vector<double> blink_intervals;
    std::vector<double> humidity_intervals;
    std::vector<double> pressure_intervals;
    std::vector<double> T1_intervals;
    std::vector<double> battery_intervals;
    for(uint8_t i = 0; i < 8; i++)
    {
        auto interval = get_timer_interval((timer)i) / 512.;
        uint8_t active_samples = 0;
        bool blink = false;
        get_active_measurements((timer)i, &active_samples, &blink);
        //std::cout << (int)active_samples << "\t" << interval << std::endl;
        if(blink)
            blink_intervals.push_back(interval);
        if(active_samples & active_measurement::pressure)
            pressure_intervals.push_back(interval);
        if(active_samples & active_measurement::humidity)
            humidity_intervals.push_back(interval);
        if(active_samples & active_measurement::T1)
            T1_intervals.push_back(interval);
        if(active_samples & active_measurement::bat)
            battery_intervals.push_back(interval);
    }
    if(blink_intervals.size())
    {
        return_str << "\tBlink intervals (seconds):\t";
        for(auto &i : blink_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(pressure_intervals.size())
    {
        return_str << "\tPressure intervals (seconds):\t";
        for(auto &i : pressure_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(humidity_intervals.size())
    {
        return_str << "\tHumidity intervals (seconds):\t";
        for(auto &i : humidity_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(T1_intervals.size())
    {
        return_str << "\tT1 intervals (seconds):\t";
        for(auto &i : T1_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(battery_intervals.size())
    {
        return_str << "\tBattery intervals (seconds):\t";
        for(auto &i : battery_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(return_str.str().size() == 0) return_str << "\tNone\n";
    return return_str.str();
}

std::string MSRTool::get_device_time_str()
{
    return get_time_str(&MSRTool::get_device_time);
}

std::string MSRTool::get_start_time_str()
{
    return get_time_str(&MSRTool::get_start_time);
}

std::string MSRTool::get_end_time_str()
{
    return get_time_str(&MSRTool::get_end_time);
}

std::string MSRTool::get_time_str(struct tm(MSRTool::*get_func)(void))
{
    struct tm _time = (this->*get_func)();
    char *_time_str = new char[100];
    strftime(_time_str, 100, timeformat, &_time);
    std::string ret_str = _time_str;
    delete[] _time_str;
    return ret_str;
}

void MSRTool::start_recording(std::string starttime_str, std::string stoptime_str, bool ringbuff)
{
    struct tm start_tm;
    struct tm stop_tm;
    auto start_tm_ptr = &start_tm;
    auto stop_tm_ptr = &stop_tm;
    if(strptime(starttime_str.c_str(), timeformat, start_tm_ptr) == nullptr)
    {
        if(starttime_str.size())
        {
            std::cout << "Incorect format in starttime string!" << std::endl;
            return;
        }
        start_tm_ptr = nullptr;
    }
    if(strptime(stoptime_str.c_str(), timeformat, stop_tm_ptr) == nullptr)
    {
        if(stoptime_str.size())
        {
            std::cout << "Incorect format in stoptime string!" << std::endl;
            return;
        }
        stop_tm_ptr = nullptr;
    }
    if(stop_tm_ptr)
    {
        if(start_tm_ptr)
            start_recording(startcondition::time_start_stop, start_tm_ptr, stop_tm_ptr, ringbuff);
        else
            start_recording(startcondition::time_stop, nullptr, stop_tm_ptr, ringbuff);
    }
    else if(start_tm_ptr)
        start_recording(startcondition::time_start, start_tm_ptr, nullptr, ringbuff);

    else start_recording(startcondition::now, nullptr, nullptr, ringbuff);
}

void MSRTool::start_recording(startcondition start_option, bool ringbuff)
{
    start_recording(start_option, nullptr, nullptr, ringbuff);
}
