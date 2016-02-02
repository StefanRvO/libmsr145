/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include "msr145_tool.hpp"

#define COMMAND_LINE_ERROR 1
#define UNHANDLED_EXCEPTION 2
namespace po = boost::program_options;

//Forward declarations
int handle_args(int argc, char const **argv,
    po::options_description &desc, po::positional_options_description &p, po::variables_map &vm);
int handle_extract_args(__attribute__((unused))po::variables_map &vm, MSRTool &msr);
int handle_start_args(po::variables_map &vm, MSRTool &msr);
int add_to_intervallist(std::vector<float> &interval_list, active_measurement::active_measurement type,
    std::vector<measure_interval_pair> &interval_type_list);

int main(int argc, char const **argv);

bool measure_interval_pair_cmp(measure_interval_pair &p1, measure_interval_pair &p2)
{
    return p1.first < p2.first;
}

int handle_args(int argc, char const **argv,
    po::options_description &desc, po::positional_options_description &p, po::variables_map &vm)
{
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    if(vm.count("help") || argc == 1)
    {
        std::cout << std::endl << "Command line tool for interfacing with the MSR145" << std::endl
        << desc << std::endl;

        std::cout << "\nFormat for time is YYYY:MM:DD--HH:MM:SS\n";
        return 0;
    }
    po::notify(vm);
    MSRTool msr(vm["device"].as<std::string>());
    if(vm.count("status"))
    {
        msr.print_status();
    }
    if(vm.count("stop"))
    {
        msr.stop_recording();
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
            std::cout << len << std::endl;
            uint8_t *response = new uint8_t[len];
            msr.send_command(cmd_vec.data(), cmd_vec.size() - 1, response, len);
            for(size_t i = 0; i < len; i++)
                printf("%02X ", response[i]);
            printf("\n");
            delete[] response;
        }
    }
    if(vm.count("format"))
    {
        std::cout << "Formating the memory. You got 5 seconds to cancel, else i will proceed." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        msr.format_memory();
    }
    if(vm.count("list"))
    {
        msr.list_recordings();
    }
    if(vm.count("extract"))
    {
        handle_extract_args(vm, msr);
    }
    if(vm.count("setname"))
    {
        msr.set_name(vm["setname"].as<std::string>());
    }
    if(vm.count("setcalibname"))
    {
        msr.set_calib_name(vm["setcalibname"].as<std::string>());
    }
    if(vm.count("setcalibdate"))
    {
        auto vec = vm["setcalibdate"].as<std::vector<uint16_t> >();
        if(vec.size() != 3)
        {
            std::cout << "setcalibdate needs excatly three aruments!" << std::endl;
            return 1;
        }
        msr.set_calibration_date(vec[0], vec[1], vec[2]);
    }
    if(vm.count("calib_pressure"))
    {
        auto vec = vm["calib_pressure"].as<std::vector<float> >();
        msr.set_calibrationdata(sampletype::pressure, vec);
    }
    if(vm.count("calib_humidity"))
    {
        auto vec = vm["calib_humidity"].as<std::vector<float> >();
        msr.set_calibrationdata(sampletype::humidity, vec);
    }
    if(vm.count("start"))
    {   //should be last arg to check
        return handle_start_args(vm, msr);
    }

    return 0;
}
int handle_extract_args(__attribute__((unused))po::variables_map &vm, MSRTool &msr)
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

int handle_start_args(po::variables_map &vm, MSRTool &msr)
{
    if(vm.count("pressure") || vm.count("humidity") || vm.count("battery") || vm.count("blink"))
    {   //recording options
        std::vector<measure_interval_pair> interval_typelist;
        if(vm.count("pressure"))
        {
            auto ist = vm["pressure"].as<std::vector<float> >();
            add_to_intervallist(ist, active_measurement::pressure, interval_typelist);
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

int add_to_intervallist(std::vector<float> &interval_list, active_measurement::active_measurement type,
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

int main(int argc, char const **argv) {
    try
    {
        po::options_description desc("Usage");
        desc.add_options()
            ("device,D", po::value<std::string>()->required(), "Serial device attached to the MSR145")
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
            ("pressure",  po::value<std::vector<float> >()->multitoken(), "Record pressure. Arguments are intervals (--start required)")
            ("humidity",  po::value<std::vector<float> >()->multitoken(), "Record humidity. Arguments are intervals(--start required)")
            ("battery",  po::value<std::vector<float> >()->multitoken(), "Record battery. Arguments are intervals(--start required)")
            ("blink",  po::value<std::vector<float> >()->multitoken(), "Blink blue led. Arguments are intervals(--start required)")
            ("setname", po::value<std::string>(), "Set the name of the device")
            ("setcalibdate", po::value<std::vector<uint16_t> >()->multitoken(), "Set the calibration date. Give three arguments, year, month, day")
            ("setcalibname", po::value<std::string>(), "Set calibration name")
            ("calib_temp_p", po::value<std::vector<float> >()->multitoken(), "set calibration settings for temperature(p)")
            ("calib_humidity", po::value<std::vector<float> >()->multitoken(), "set calibration settings for humidity")

            ;
        po::positional_options_description p;
        p.add("device", 1);
        po::variables_map vm;
        try
        {
            return handle_args(argc, argv, desc, p, vm);
        }
        catch(po::error &e)
        {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << desc << std::endl;
            return COMMAND_LINE_ERROR;
        }
    }
    catch(std::exception &e)
    {
        std::cerr << "An error occured:\n\n" << e.what()
        << "\nBailing out!" << std::endl;
        return UNHANDLED_EXCEPTION;
    }
    return 0;
}
