#include <boost/program_options.hpp>
#include <iostream>

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
    }
    po::notify(vm);


    return 0;
}

int main(int argc, char const **argv) {
    try
    {
        po::options_description desc("Options");
        desc.add_options()
            ("help,h", "Print help messages")
            ("status,s", "Print current settings of the MSR145")
            ("stop", "Stop recording samples")
            ("start", "Start recording samples")
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
        std::cerr << "Unhandled exception: " << e.what()
        << ", bailing out!" << std::endl;
        return UNHANDLED_EXCEPTION;
    }
    return 0;
}
