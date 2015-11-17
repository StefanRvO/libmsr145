/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#pragma once
#include <boost/asio.hpp>
#include <string>
#include <ctime>
#include <vector>
#define MSR_BUAD_RATE 9600
#define MSR_STOP_BITS boost::asio::serial_port_base::stop_bits::one
#define MSR_WORD_LENGHT 8
#
struct rec_entry
{
    uint8_t address;
    struct tm time;
    uint16_t lenght;
};
struct sample
{
    uint32_t RH; //Realative hydration in 1:10000

};
class MSRDevice
{
    private:
        boost::asio::serial_port *port;
        boost::asio::io_service ioservice;
        std::string portname;
    public:
        MSRDevice(std::string _portname);
        ~MSRDevice();
        std::string getSerialNumber();
        std::string getName();
        struct tm getTime();
        void setName(std::string name);
        std::vector<rec_entry> getRecordings(); //only work when recording is not active
    public:
        void sendcommand(uint8_t * command, size_t command_lenght, uint8_t *out, size_t out_lenght);
        void sendraw(uint8_t * command, size_t command_lenght, uint8_t *out, size_t out_lenght);
        uint8_t calcChecksum(uint8_t *data, size_t lenght);
        std::vector<uint8_t> readRecording(rec_entry record);
        void set_baud230400();


};
