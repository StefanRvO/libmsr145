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
    pressure = 0x0,
    T_pressure = 0x1,
    humidity = 0x5,
    T_humidity = 0x6,
    bat = 0xE,
    ext1 = 0xA,
    ext2 = 0xB,
    ext3 = 0xC,
    ext4 = 0x7,
    timestamp = 0xF,
    end = 0xFFFF,
    none = 0x55, //Just a placeholder for none for use in getSensorData
};

struct rec_entry
{
    uint16_t address;
    struct tm time;
    uint16_t lenght;
};

struct sample
{
    sampletype type;
    int16_t value;
    uint64_t timestamp; //this is the time since the start of the recording in 1/512 seconds
    uint32_t rawsample; //for debugging
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
        void set_baud(uint32_t baudrate);
        sample convertToSample(uint8_t *sample_ptr, uint64_t *total_time);
        rec_entry create_rec_entry(uint8_t *response_ptr, uint16_t start_addr, uint16_t end_addr);
        void updateSensors();
        void getSensorData(int16_t *returnvalues, sampletype type1 = sampletype::none,
            sampletype type2 = sampletype::none, sampletype type3 = sampletype::none);


};
