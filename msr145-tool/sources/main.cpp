#include "options_handler.hpp"
#include <boost/filesystem.hpp>
int main(int argc, char const **argv) {
    options_handler o_handler;
    try
    {
        //open the msr.
        //open the msr.
        if(!boost::filesystem::exists(argv[1]))
        {
            throw po::error("The device don't exists!");
        }
        MSRTool msr(argv[1]);
        try
        {
            o_handler.handle_args(argc, argv, &msr);
        }
        catch(po::error &e)
        {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << *(o_handler.desc) << std::endl;
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
