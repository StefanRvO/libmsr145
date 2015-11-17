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

enum sampletype
{
    unknown1 = 0x00,
    unknown2 = 0xE0,
    unknown3 = 0x50,
    rel_hydro = 0x60, //verified
    T_rel_hydro = 0x02, //verified
    temp = 0x111/*not a real ID, but all of the temps are set to this when downloading samples*/,
    temp_alt0 = 0x57, //verified
    temp_alt1 = 0x70, //verified
    temp_alt2 = 0x77, //verified, it's however very strange that there is three id's for temp-
    pressure = 0x10, //Verified
    ext1 = 0xA0,
    ext2 = 0xB0,
    ext3 = 0xC0,
    ext4 = 0xD0,
    end = 0xFF,
};

struct rec_entry
{
    uint8_t address;
    struct tm time;
    uint16_t lenght;
};

struct sample
{
    sampletype type;
    uint32_t value;
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
        std::vector<rec_entry> getRecordinglist(); //only work when recording is not active
        std::vector<sample> getSamples(rec_entry record);
    public:
        void sendcommand(uint8_t * command, size_t command_lenght, uint8_t *out, size_t out_lenght);
        void sendraw(uint8_t * command, size_t command_lenght, uint8_t *out, size_t out_lenght);
        uint8_t calcChecksum(uint8_t *data, size_t lenght);
        std::vector<uint8_t> getRawRecording(rec_entry record);
        void set_baud230400();
        sample convertToSample(uint8_t *sample_ptr);
        rec_entry create_rec_entry(uint8_t *response_ptr, uint16_t start_addr, uint16_t end_addr);

};
