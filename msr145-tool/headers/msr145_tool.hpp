#pragma once
#include "libmsr145.hpp"
#include <string>
class MSRTool : public MSRDevice
{
    public:
        MSRTool(std::string _portname) : MSR_Base(_portname), MSRDevice(_portname)
            {}
        virtual ~MSRTool()
            {}
        virtual void print_status();
        using MSR_Writer::start_recording;
        virtual void start_recording(std::string starttime_str, std::string stoptime_str, bool ringbuff);
        virtual void start_recording(startcondition start_option, bool ringbuff); //only to use for push options
        virtual std::string get_time_str(struct tm(MSRTool::*get_func)(void));
        virtual std::string get_device_time_str();
        virtual std::string get_start_time_str();
        virtual std::string get_end_time_str();
        virtual std::string get_interval_string();
};
