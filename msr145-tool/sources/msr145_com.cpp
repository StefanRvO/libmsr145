/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "options_handler.hpp"
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>
std::vector<std::string> tokenize(const std::string& input)
{
  typedef boost::escaped_list_separator<char> separator_type;
  separator_type separator("\\",    // The escape characters.
                           "= ",    // The separator characters.
                           "\"\'"); // The quote characters.

  // Tokenize the intput.
  boost::tokenizer<separator_type> tokens(input, separator);

  // Copy non-empty tokens from the tokenizer into the result.
  std::vector<std::string> result;
  copy_if(tokens.begin(), tokens.end(), std::back_inserter(result),
          !boost::bind(&std::string::empty, _1));
  return result;
}

int main(__attribute__((unused))int argc, __attribute__((unused)) char const *argv[]) {
    options_handler o_handler;
    std::string command;
    try
    {
        //open the msr.
        MSRTool msr(argv[1]);
        while(std::getline(std::cin, command))
        {
            std::cout << command << std::endl;
            try
            {
                auto tokens = tokenize(command);
                o_handler.handle_tokens(tokens, &msr);
            }
            catch(po::error &e)
            {
                std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
                std::cerr << *(o_handler.desc) << std::endl;
                return COMMAND_LINE_ERROR;
            }
        }
    }
    catch(std::exception &e)
    {
        std::cerr << "An error occured:\n\n" << e.what()
        << "\nBailing out!" << std::endl;
        return UNHANDLED_EXCEPTION;
    }
}
