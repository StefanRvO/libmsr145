#include "msr145_tool.hpp"
#include <ctime>
#include <iostream>
#include <sstream>
static const char *timeformat = "%Y:%m:%d--%H:%M:%S";


void MSRTool::print_status()
{
    std::cout << "Device Status:" << std::endl;
    std::cout << "Serial number: " << get_serial() << std::endl;
    std::cout << "Device time: " << get_device_time_str() << std::endl;
    std::cout << "Device is recording: " << is_recording() << std::endl;
    std::cout << get_interval_string() << std::endl;

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
        return_str << "Blink intervals (seconds):\t";
        for(auto &i : blink_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(pressure_intervals.size())
    {
        return_str << "Pressure intervals (seconds):\t";
        for(auto &i : pressure_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(humidity_intervals.size())
    {
        return_str << "Humidity intervals (seconds):\t";
        for(auto &i : humidity_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(T1_intervals.size())
    {
        return_str << "T1 intervals (seconds):\t";
        for(auto &i : T1_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
    if(battery_intervals.size())
    {
        return_str << "Battery intervals (seconds):\t";
        for(auto &i : battery_intervals) return_str << i << "\t";
        return_str << std::endl;
    }
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
            start_recording(startcondition::time_stop, start_tm_ptr, stop_tm_ptr, ringbuff);
    }
    else if(start_tm_ptr)
        start_recording(startcondition::time_start, start_tm_ptr, stop_tm_ptr, ringbuff);

    else start_recording(startcondition::now, nullptr, nullptr, ringbuff);
}

void MSRTool::start_recording(startcondition start_option, bool ringbuff)
{
    start_recording(start_option, nullptr, nullptr, ringbuff);
}
