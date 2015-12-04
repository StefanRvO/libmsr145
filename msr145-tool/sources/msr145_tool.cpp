/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "msr145_tool.hpp"
#include <ctime>
#include <iostream>
#include <sstream>
#include <algorithm>

static const char *timeformat = "%Y:%m:%d--%H:%M:%S";

bool sampletype_cmp(sampletype &type1, sampletype &type2)
{
    return type1 < type2;
}

bool sample_cmp(sample &s1, sample &s2)
{
    if(s1.timestamp == s2.timestamp)
        return sampletype_cmp(s1.type, s2.type);
    return s1.timestamp < s2.timestamp;
}

void MSRTool::print_status()
{
    update_sensors();
    std::cout << "Device Status:" << std::endl << std::endl;
    std::cout << "Serial Number:\t " << get_serial() << std::endl;
    std::cout << "Device Name:\t " << get_name() << std::endl;
    std::cout << "Device time:\t " << get_device_time_str() << std::endl;
    std::cout << "Device is recording:\t " << is_recording() << std::endl;
    bool marker_on, alarm_confirm_on;
    get_marker_setting(&marker_on, &alarm_confirm_on);
    std::cout << "Marker:\t\t\t" << marker_on << std::endl;
    std::cout << "Alarm confirm:\t" <<  alarm_confirm_on << std::endl;
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
        std::cout << get_sensor_str(sensor_to_poll[i], sensor_readings[i]);
    std::cout << std::endl;
    std::cout << get_start_settings_str() << std::endl;
    std::cout << "Limit settings:" << std::endl;
    std::cout << get_limits_str() << std::endl;
    std::cout << "Calibration settings:" << std::endl;
    uint8_t year, month, day;
    std::cout << "\tCalibration name:\t" << get_calibration_name(&year, &month, &day) << std::endl;
    std::cout << "\tCalibration date:\t" << (uint16_t)year + 2000 << ":" << month + 1 << ":" << day + 1 << std::endl;
    std::cout << get_calibration_str() << std::endl;

}

void MSRTool::set_name(std::string name)
{
    //First, read in calibration date and callibration name, as we need to set them at the same time.
    uint8_t year, month, day;
    auto calib_name = get_calibration_name(&year, &month, &day);
    //write them
    set_names_and_calibration_date(name, calib_name, year, month, day);
}

void MSRTool::set_calibration_date(uint16_t year, uint16_t month, uint16_t day)
{
    //read names first. We need to set them at the same time
    auto calib_name = get_calibration_name();
    auto device_name = get_name();
    std::cout << year - 2000 << " " << month - 1 << " " << day - 1 << std::endl;
    set_names_and_calibration_date(device_name, calib_name, year - 2000, month - 1, day - 1);
}

void MSRTool::set_measurement_and_timers(std::vector<measure_interval_pair> interval_typelist)
{
    uint8_t timer = 0;
    for(auto &interval : interval_typelist)
        if(interval.first < 1/256.)
        {
            std::cout << "Sorry, can't have shorter intervals than " << 1/256. << "seconds\n";
            return;
        }
    if(interval_typelist.size() > 8)
    {
        std::cout << "Sorry, but can't have more than 8 different intervals!\n";
        return;
    }

    for(auto &interval : interval_typelist)
    {
        bool blink = false;
        bool active = false;
        uint8_t bitmask = 0x00;
        for(auto &type : interval.second)
        {
            if(type == active_measurement::blink) blink = true;
            else bitmask |= type;
            active = true;
        }
        uint32_t timer_interval = interval.first * 512; //interval is in 1/512 seconds and rounded down.
        set_timer_interval(timer, timer_interval);
        set_timer_measurements(timer, bitmask, blink, active);
        timer++;
    }
    for(; timer < 8; timer++) //zero out the remaining timers
    {
        set_timer_interval(timer, 0);
        set_timer_measurements(timer, 0, 0, 0);
    }
}

void MSRTool::list_recordings()
{
    auto rec_list = get_rec_list();
    char *date_str = new char[100];
    std::cout << "Recordings on device:\n";
    std::cout << "Number:\t\t\tDate:\t\t\tPage Length:\n\n";
    size_t i = 0;
    for(auto &rec : rec_list)
    {
        strftime(date_str, 100, timeformat, &rec.time);
        std::cout << i++ << "\t\t\t" << date_str << "\t" << rec.length << std::endl;
    }
    delete[] date_str;
}

void MSRTool::extract_record(uint32_t rec_num, std::string seperator, std::ostream &out_stream)
{
    //First, get list of recordings
    char *date_str = new char[100];
    auto rec_list = get_rec_list(rec_num + 1);
    if(rec_list.size() < rec_num + 1)
    {
        std::cout << "The requested recording is not on the device!" << std::endl;
    }
    //Grab the samples
    strftime(date_str, 100, timeformat, &(rec_list[rec_num].time));
    auto samples = get_samples(rec_list[rec_num]);
    out_stream << "Samples collected from an MSR145" << std::endl;
    out_stream << "Sampling start time: " << date_str << std::endl;
    out_stream << std::endl;
    out_stream << create_csv(samples, seperator);
}

std::string MSRTool::create_csv(std::vector<sample> &samples, std::string &seperator)
{
    std::stringstream csv;
    std::vector<sampletype> sampletypes;
    //Create header with info about types and units
    for(auto &sample : samples)
    {
        if(std::find(sampletypes.begin(), sampletypes.end(), sample.type) == std::end(sampletypes))
        {
            sampletypes.push_back(sample.type);
        }
    }
    std::sort(sampletypes.begin(), sampletypes.end(), sampletype_cmp);
    csv << "Timestamp (s)" << seperator;
    for(auto &type : sampletypes)
    {
        std::string type_str, unit_str;
        get_type_str(type, type_str, unit_str);
        csv << type_str << " (" << unit_str << ")" << seperator;
    }
    csv << std::endl;
    //Create the sample lines
    //sort samples according to time and type
    std::sort(samples.begin(), samples.end(), sample_cmp);
    std::unique(samples.begin(), samples.end(), sample_cmp);
    uint64_t last_stamp = 0xFFFFFFFFFFFFFFFF;
    size_t placement = 0;
    for(auto &sample : samples)
    {
        //if(sample.timestamp == 5457) printf("%08X\n", sample.rawsample);
        if(sample.timestamp != last_stamp)
        {
            placement = 0;
            last_stamp = sample.timestamp;
        }
        if(placement == 0)
        {
            csv << std::endl;
            csv << sample.timestamp / double((1 << 17))  << seperator;
            placement++;
        }
        while(sample.type != sampletypes[placement - 1])
        {
            placement++;
            csv << seperator;
        }
        csv << convert_to_unit(sample.type, sample.value);
    }
    return csv.str();
}

void MSRTool::get_type_str(sampletype type, std::string &type_str, std::string &unit_str)
{
    switch(type)
    {
        case pressure:
            type_str = "Pressure";
            unit_str = "mbar";
            break;
        case T_pressure:
            type_str = "Temperature(p)";
            unit_str = "C";
            break;
        case humidity:
            type_str = "Humidity";
            unit_str = "%";
            break;
        case T_humidity:
            type_str = "Temperature(RH)";
            unit_str = "C";
            break;
        case bat:
            type_str = "Battery";
            unit_str = "V";
            break;
        default:
            assert(false); //this should not happen
    }
}

std::string MSRTool::get_calibration_str()
{   //Return string with calibraion data
    std::stringstream ret_str;

    for(uint8_t i = 0; i < 0xF; i++) //run through all the types
    {
        uint16_t point_1_target, point_1_actual, point_2_target, point_2_actual;
        switch(i)
        {
            case sampletype::pressure: case sampletype::T_pressure:
            case sampletype::humidity: case sampletype::T_humidity:
            case sampletype::bat:
            {
                get_calibrationdata((sampletype)i, &point_1_target, &point_1_actual,
                    &point_2_target, &point_2_actual);
                std::string type_str, unit_str;
                std::stringstream tmp_ss;
                get_type_str((sampletype)i, type_str, unit_str);
                    tmp_ss << "\tCalibration for " << type_str << " (" << unit_str << ") :";
                    while(tmp_ss.str().size() < 40  ) tmp_ss << " ";
                    tmp_ss << "P1_Target: " << convert_to_unit((sampletype)i, point_1_target) << " ";
                    tmp_ss << "P1_Actual: " << convert_to_unit((sampletype)i, point_1_actual) << " ";
                    tmp_ss << "P2_Target: " << convert_to_unit((sampletype)i, point_2_target) << " ";
                    tmp_ss << "P2_Actual: " << convert_to_unit((sampletype)i, point_2_actual) << " ";
                    ret_str << tmp_ss.str() << std::endl;
                    break;
            }
        }
    }
    return ret_str.str();
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
        return std::string("\tNone\n");
}

std::string MSRTool::get_sample_limit_str(sampletype type)
{
    std::stringstream ret_str;
    ret_str << "\tLimits for ";
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

std::string MSRTool::get_sensor_str(sampletype type, uint16_t value)
{
    std::stringstream ret_str;
    std::string type_str, unit_str;
    get_type_str(type, type_str, unit_str);
    ret_str << "\t" << type_str << " (" << unit_str << "):";
    while(ret_str.str().size() < 30) ret_str << " ";
    ret_str << convert_to_unit(type, value);
    ret_str << std::endl;

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
        auto interval = get_timer_interval(i) / 512.;
        uint8_t active_samples = 0;
        bool blink = false;
        get_active_measurements(i, &active_samples, &blink);
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
