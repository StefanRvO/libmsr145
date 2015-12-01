#include "msr145_tool.hpp"
#include <ctime>
void MSRTool::print_status()
{

}

void MSRTool::start_recording(std::string starttime_str, std::string stoptime_str, bool ringbuff)
{
    const char *timeformat = "%Y:%m:%d:%H:%M:%S";
    struct tm start_tm;
    struct tm stop_tm;
    auto start_tm_ptr = &start_tm;
    auto stop_tm_ptr = &stop_tm;
    if(strptime(starttime_str.c_str(), timeformat, start_tm_ptr) == nullptr)
        start_tm_ptr = nullptr;
    if(strptime(stoptime_str.c_str(), timeformat, stop_tm_ptr) == nullptr)
        stop_tm_ptr = nullptr;
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
