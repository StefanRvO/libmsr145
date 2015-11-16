#pragma once
#include <boost/asio.hpp>
#include <string>
#include <ctime>
#include <vector>
#define MSR_BUAD_RATE 9600
#define MSR_STOP_BITS boost::asio::serial_port_base::stop_bits::one
#define MSR_WORD_LENGHT 8
struct rec_entry
{
    uint8_t address;
    struct tm time;
    uint16_t lenght;
};

class MSRDevice
{
    private:
        boost::asio::serial_port *port;
        boost::asio::io_service ioservice;
    public:
        MSRDevice(std::string portname);
        ~MSRDevice();
        std::string getSerialNumber();
        std::string getName();
        struct tm getTime();
        void setName(std::string name);
        std::vector<rec_entry> getRecordings(); //only work when recording is not active
    public:
        void sendcommand(uint8_t * command, size_t command_lenght, uint8_t *out, size_t out_lenght);
        uint8_t calcChecksum(uint8_t *data, size_t lenght);
        std::vector<uint8_t> readRecording(rec_entry record);

};
