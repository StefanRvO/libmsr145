/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "options_handler.hpp"

bool options_handler::measure_interval_pair_cmp(measure_interval_pair &p1, measure_interval_pair &p2)
{
    return p1.first < p2.first;
}

options_handler::~options_handler()
{
    delete desc;
}

options_handler::options_handler()
{
    desc = new po::options_description("Usage");
    desc->add_options()
        ("help,h", "Print help messages")
        ("stop", "Stop recording samples")
        ("start", "Start recording samples")
        ("ringbuffer", "Should the device use a ringbuffer when running out of memory? (--start required)")
        ("starttime",po::value<std::string>(), "Time where recording starts (--start required)")
        ("stoptime",po::value<std::string>(), "Time where recording stops (--start required)")
        ("startpush", "Start and stop recording on buttonpush(--start required), overwrites starttime and stoptime")
        ("startstoppush", "Start and stop recording on buttonpush(--start required), overwrites starttime and stoptime")
        ("status,s", "Print current settings of the MSR145")
        ("rawcmd", po::value<std::vector<std::string> >()->multitoken(), "Send a raw command to the device and read the answer. last argument in list is lenght of answer")
        ("format", "Format the memory")
        ("list,l", "List the recordings on the device.")
        ("extract,X", po::value<uint32_t>(),     "extract a recording from the device, the record number is given as argument")
        ("seperator", po::value<std::string>(), "The seperator used when extracting")
        ("outfile,o", po::value<std::string>(), "The file extracted to, default is stdout")
        ("pressure",  po::value<std::vector<float> >()->multitoken(), "Record pressure. Arguments are intervals (--setsampling required)")
        ("light",  po::value<std::vector<float> >()->multitoken(), "Record light level. Arguments are intervals (--setsampling required)")
        ("humidity",  po::value<std::vector<float> >()->multitoken(), "Record humidity. Arguments are intervals (--setsampling required)")
        ("battery",  po::value<std::vector<float> >()->multitoken(), "Record battery. Arguments are intervals (--setsampling required)")
        ("blink",  po::value<std::vector<float> >()->multitoken(), "Blink blue led. Arguments are intervals (--setsampling required)")
        ("setsampling", "Set the sample rates for the different samples")
        ("setname", po::value<std::string>(), "Set the name of the device")
        ("setcalibdate", po::value<std::vector<uint16_t> >()->multitoken(), "Set the calibration date. Give three arguments, year, month, day")
        ("setcalibname", po::value<std::string>(), "Set calibration name")
        ("calib_humidity", po::value<std::vector<float> >()->multitoken(), "set calibration settings for humidity (somewhat buggy)")
        ("calib_temp_humidity", po::value<std::vector<float> >()->multitoken(), "set calibration settings for temperature (humidity sensor) (somewhat buggy)")
        ("calib_temp_T", po::value<std::vector<float> >()->multitoken(), "set calibration settings for temperature (T sensor) (somewhat buggy)")
        ("settime", po::value<std::string>()->implicit_value(""), "Set the device time. If no argument is given, the time is set to current time.")
        ("setlimit", "Set a limit setting. Requires additional arguments for setting the actual limit")
        ("limittype", po::value<std::string>(), "Which type of limit to set. Could be '(L)light', '(p)pressure', '(T_p)temp_pressure', '(RH)humidity' (humidity limits may be buggy) or '(T_RH)temp_humidity'")
        ("alarmlimit", po::value<std::string>(), "Set an alarm limit, types are: 'none(default), 'S<L1', 'S>L1', 'L1<S<L2' and 'S<L1||S>L2'")
        ("recordlimit", po::value<std::string>(), "Set an record limit, types are: 'none(default), 'S<L2', 'S>L2', 'L1<S<L2', 'S<L1||S>L2', 'START>L1,STOP<L2' and START<L1,STOP>L2")
        ("limit1", po::value<float>(), "sets L1 for the given type, 0 is default")
        ("limit2", po::value<float>(), "sets L2 for the given type, 0 is default")
        ("clearlimits", "clear all limits")
        ("getsensors", po::value<std::vector<std::string> >()->multitoken(), "get the newest reading from the sensors. Arguments are '(L)light', '(p)pressure', '(T_p)temp_pressure', '(RH)humidity', '(T_RH)temp_humidity' or B(battery)")
        ("set_light_unit", po::value<std::string>(), "Set the name of the unit for the light sensor")
        ;
}

int options_handler::handle_args(int argc, char const **argv, MSRTool *msr)
{
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, *desc), vm);
    return handle_command(vm, msr);
}

int options_handler::handle_tokens(std::vector<std::string> &tokens, MSRTool *msr)
{
    po::variables_map vm;
    po::store(po::command_line_parser(tokens).options(*desc).run(), vm);
    return handle_command(vm, msr);
}

int options_handler::handle_command(po::variables_map &vm, MSRTool *msr)
{
    if(vm.count("help"))
    {
        std::cout << std::endl << "Command line tool for interfacing with the MSR145" << std::endl
        << *desc << std::endl;

        std::cout << "\nFormat for time is YYYY:MM:DD--HH:MM:SS\n";
        return 0;
    }
    po::notify(vm);
    if(vm.count("status"))
    {
        msr->print_status();
    }
    if(vm.count("getsensors"))
    {
        handle_get_sensors(vm, *msr);
    }
    if(vm.count("set_light_unit"))
    {
        msr->set_L1_unit(vm["set_light_unit"].as<std::string>());
    }
    if(vm.count("stop"))
    {
        msr->stop_recording();
    }
    if(vm.count("rawcmd"))
    {
        std::vector<uint8_t> cmd_vec;
        auto vec = vm["rawcmd"].as<std::vector<std::string> >();
        for(size_t i = 0; i < vec.size(); i++)
            cmd_vec.push_back(std::stoul(vec[i], nullptr, 16));
        if(cmd_vec.size())
        {
            size_t len = std::stoul(vec.back(), nullptr, 10);
            //std::cout << len << std::endl;
            uint8_t *response = new uint8_t[len];
            msr->send_command(cmd_vec.data(), cmd_vec.size() - 1, response, len);
            for(size_t i = 0; i < len; i++)
                printf("%02X ", response[i]);
            printf("\n");
            delete[] response;
        }
    }
    if(vm.count("format"))
    {
        std::cout << "Formating the memory. You got 5 seconds to cancel, else I will proceed!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        msr->format_memory();
    }
    if(vm.count("list"))
    {
        msr->list_recordings();
    }
    if(vm.count("extract"))
    {
        handle_extract_args(vm, *msr);
    }
    if(vm.count("setname"))
    {
        msr->set_name(vm["setname"].as<std::string>());
    }
    if(vm.count("settime"))
    {
        msr->set_time(vm["settime"].as<std::string>());
    }
    if(vm.count("setcalibname"))
    {
        msr->set_calib_name(vm["setcalibname"].as<std::string>());
    }
    if(vm.count("setcalibdate"))
    {
        auto vec = vm["setcalibdate"].as<std::vector<uint16_t> >();
        if(vec.size() != 3)
        {
            throw po::error("setcalibdate needs excatly three aruments!");
        }
        msr->set_calibration_date(vec[0], vec[1], vec[2]);
    }
    if(vm.count("calib_temp_humidity"))
    {
        auto vec = vm["calib_temp_humidity"].as<std::vector<float> >();
        msr->set_calibrationpoints(active_calibrations::temperature_RH, vec);
    }
    if(vm.count("calib_humidity"))
    {
        auto vec = vm["calib_humidity"].as<std::vector<float> >();
        msr->set_calibrationpoints(active_calibrations::humidity, vec);
    }
    if(vm.count("calib_temp_T"))
    {
        auto vec = vm["calib_temp_T"].as<std::vector<float> >();
        msr->set_calibrationpoints(active_calibrations::temperature_T, vec);
    }
    if(vm.count("setlimit"))
    {
        handlelimits(vm, *msr);
    }
    if(vm.count("clearlimits"))
    {
        msr->reset_limits();
    }
    if(vm.count("setsampling"))
    {
        handle_sampling_args(vm, *msr);
    }
    if(vm.count("start"))
    {   //should be last arg to check
        return handle_start_args(vm, *msr);
    }

    return 0;
}

void options_handler::handlelimits(po::variables_map &vm, MSRTool &msr)
{
    if(!vm.count("limittype"))
    {
        std::cout << "no limittype set!" << std::endl;
        return;
    }
    auto limittype_str = vm["limittype"].as<std::string>();
    sampletype limittype;
    if(limittype_str == "p" or limittype_str == "pressure")
        limittype = pressure;
    else if(limittype_str == "T_p" or limittype_str == "temp_pressure")
        limittype = T_pressure;
    else if(limittype_str == "RH" or limittype_str == "humidity")
        limittype = humidity;
    else if(limittype_str == "T_RH" or limittype_str == "temp_humidity")
        limittype = T_humidity;
    else if(limittype_str == "T_p" or limittype_str == "temp_pressure")
        limittype = T_pressure;
    else if(limittype_str == "L" or limittype_str == "light")
        limittype = light;
    else
    {
        throw po::error("limittype invalid!");
        return;
    }
    float limit1 = 0, limit2 = 0;
    if(vm.count("limit1")) limit1 = vm["limit1"].as<float>();
    if(vm.count("limit2")) limit2 = vm["limit2"].as<float>();

    limit_setting alarm_limit = no_limit;
    limit_setting rec_limit = no_limit;
    if(vm.count("alarmlimit")) alarm_limit = parse_alarm_limit(vm["alarmlimit"].as<std::string>());
    if(vm.count("recordlimit")) rec_limit = parse_recording_limit(vm["recordlimit"].as<std::string>());
    msr.set_limit(limittype, limit1, limit2, rec_limit, alarm_limit);
}

limit_setting options_handler::parse_alarm_limit(std::string limit_str)
{
    limit_setting setting = no_limit;
    if(limit_str == "S<L1") setting = alarm_less_limit1;
    else if(limit_str == "S>L1") setting = alarm_more_limit1;
    else if(limit_str == "L1<S<L2") setting = alarm_more_limit1;
    else if(limit_str == "S<L1||S>L2") setting = alarm_more_limit1;
    else if(limit_str == "none") setting = no_limit;
    else
    {
        throw po::error("Invalid limit setting for alarm!");
    }
    return setting;
}

limit_setting options_handler::parse_recording_limit(__attribute__((unused))std::string limit_str)
{
    limit_setting setting = no_limit;
    if(limit_str == "S<L2") setting = rec_less_limit2;
    else if(limit_str == "S>L2") setting = rec_more_limit2;
    else if(limit_str == "L1<S<L2") setting = rec_more_limit1_and_less_limit2;
    else if(limit_str == "S<L1||S>L2") setting = rec_less_limit1_or_more_limit2;
    else if(limit_str == "START>L1,STOP<L2") setting = rec_start_more_limit1_stop_less_limit2;
    else if(limit_str == "START<L1,STOP>L2") setting = rec_start_less_limit1_stop_more_limit2;
    else if(limit_str == "none") setting = no_limit;
    else
    {
        throw po::error("Invalid limit setting for record!");
    }
    return setting;
}

void options_handler::handle_get_sensors(po::variables_map &vm, MSRTool &msr)
{
    auto strings = vm["getsensors"].as<std::vector<std::string> >();
    std::vector<sampletype> sensor_to_poll;
    for(auto string : strings)
    {
        if(string == "L" || string == "light")
            sensor_to_poll.push_back(sampletype::light);
        else if(string == "p" || string == "pressure")
            sensor_to_poll.push_back(sampletype::pressure);
        else if(string == "T_p" || string == "temp_pressure")
            sensor_to_poll.push_back(sampletype::T_pressure);
        else if(string == "RH" || string == "humidity")
            sensor_to_poll.push_back(sampletype::humidity);
        else if(string == "T_RH" || string == "temp_humidity")
            sensor_to_poll.push_back(sampletype::T_humidity);
        else if(string == "B" || string == "battery")
            sensor_to_poll.push_back(sampletype::bat);
        else
            std::cerr << "Invalid sampletype selected" << std::endl;
    }
    if(sensor_to_poll.size())
        msr.print_sensors(sensor_to_poll);
}

int options_handler::handle_extract_args(po::variables_map &vm, MSRTool &msr)
{
    std::string seperator = ",";
    std::filebuf fb;
    std::ostream out_stream(std::cout.rdbuf());
    if(vm.count("seperator"))
    {
        seperator = vm["seperator"].as<std::string>();
    }
    if(vm.count("outfile"))
    {
        fb.open(vm["outfile"].as<std::string>(), std::ios::out);
        out_stream.rdbuf(&fb);
    }
    msr.extract_record(vm["extract"].as<uint32_t>(), seperator, out_stream);
    return 0;
}

void options_handler::handle_sampling_args(po::variables_map &vm, MSRTool &msr)
{
    if(vm.count("pressure") || vm.count("humidity") || vm.count("battery") || vm.count("blink") || vm.count("light"))
    {   //recording options
        std::vector<measure_interval_pair> interval_typelist;
        if(vm.count("pressure"))
        {
            auto ist = vm["pressure"].as<std::vector<float> >();
            add_to_intervallist(ist, active_measurement::pressure, interval_typelist);
        }
        if(vm.count("light"))
        {
            auto ist = vm["light"].as<std::vector<float> >();
            add_to_intervallist(ist, active_measurement::light, interval_typelist);
        }
        if(vm.count("humidity"))
        {
            auto list = vm["humidity"].as<std::vector<float> >();
            add_to_intervallist(list, active_measurement::humidity, interval_typelist);
        }
        if(vm.count("battery"))
        {
            auto list = vm["battery"].as<std::vector<float> >();
            add_to_intervallist(list, active_measurement::bat, interval_typelist);
        }
        if(vm.count("blink"))
        {
            auto list = vm["blink"].as<std::vector<float> >();
            add_to_intervallist(list, active_measurement::blink, interval_typelist);
        }
        msr.set_measurement_and_timers(interval_typelist);
    }
}

int options_handler::handle_start_args(po::variables_map &vm, MSRTool &msr)
{
    std::string starttime_str;
    std::string stoptime_str;
    bool ringbuff = vm.count("ringbuffer");
    if(vm.count("startstoppush"))
    {
        msr.start_recording(startcondition::push_start_stop, ringbuff);
        return 0;
    }
    else if(vm.count("startpush"))
    {
        msr.start_recording(startcondition::push_start, ringbuff);
        return 0;
    }
    if(vm.count("starttime"))
    {
        starttime_str = vm["starttime"].as<std::string>();
    }
    if(vm.count("stoptime"))
    {
        stoptime_str = vm["stoptime"].as<std::string>();
    }
    msr.start_recording(starttime_str, stoptime_str, ringbuff);
    return 0;
}

int options_handler::add_to_intervallist(std::vector<float> &interval_list, active_measurement::active_measurement type,
    std::vector<measure_interval_pair> &interval_type_list)
{
    for(auto interval : interval_list)
    {
        //find the interval in the interval_type list
        measure_interval_pair *interval_ptr = nullptr;
        for(auto &num : interval_type_list)
        {
            if(num.first == interval)
            {
                interval_ptr = &num;
                break;
            }
        }
        //put in a new interval if it doesn't exist.
        if(interval_ptr == nullptr)
        {
            measure_interval_pair new_pair;
            new_pair.first = interval;
            new_pair.second.push_back(type);
            interval_type_list.push_back(new_pair);
        }
        else
        {
            interval_ptr->second.push_back(type);
        }
    }
    return 0;
}
