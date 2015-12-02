/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

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
        virtual float convert_to_unit(sampletype type, uint16_t value);
        virtual std::string get_sensor_string(sampletype type, uint16_t value);
        virtual std::string get_start_settings_str();
        virtual std::string get_sample_limit_str(sampletype type);
        virtual std::string get_limits_str();
};
