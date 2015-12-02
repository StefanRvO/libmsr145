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
#include "msr145_tool.hpp"

#define COMMAND_LINE_ERROR 1
#define UNHANDLED_EXCEPTION 2
namespace po = boost::program_options;


int handle_args(int argc, char const **argv,
    po::options_description &desc, po::variables_map &vm)
{
    po::store(po::parse_command_line(argc, argv, desc), vm);
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
            uint8_t *response = new uint8_t[cmd_vec.back()];
            msr.send_command(cmd_vec.data(), cmd_vec.size() - 1, response, cmd_vec.back());
            for(size_t i = 0; i < cmd_vec.back(); i++)
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

    if(vm.count("start"))
    {   //should be last arg to check
        std::string starttime_str;
        std::string stoptime_str;
        std::string startoption;
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

    return 0;
}

int main(int argc, char const **argv) {
    try
    {
        po::options_description desc("Options");
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
            ;
        po::variables_map vm;
        try
        {
            return handle_args(argc, argv, desc, vm);
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
