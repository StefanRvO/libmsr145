/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <stefan@stefanrvo.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "libmsr145.hpp"
#include <string>
#include <iostream>
#include <cstdio>
#include <boost/crc.hpp>
using namespace boost::asio;

//sends the given command to the MSR145 and read out_lenght number of bytes from it into out
uint8_t MSRDevice::calcChecksum(uint8_t *data, size_t lenght)
{
    //The MSR145 uses the dallas 8-bit CRC as checksum. Here i use boost to calculate it.
    boost::crc_optimal<8, 0x31, 0x00, 0x00, true, true> crc;
    crc.process_bytes(data, lenght);
    return crc.checksum();
}
void MSRDevice::sendcommand(uint8_t *command, size_t command_lenght,
                            uint8_t *out, size_t out_lenght)
{
    auto checksum = calcChecksum(command, command_lenght);
    write(*(this->port), buffer(command, command_lenght));
    write(*(this->port), buffer(&checksum, sizeof(checksum)));
    size_t read_bytes = read(*(this->port), buffer(out, out_lenght), transfer_exactly(out_lenght));
    assert(read_bytes == out_lenght);
    assert(out_lenght == 0 || (out[out_lenght - 1] == calcChecksum(out, out_lenght - 1)));
}

MSRDevice::MSRDevice(std::string portname)
{
    //open the port and setup baudrate, stop bits and word lenght
    this->port = new serial_port(ioservice, portname);
    this->port->set_option(serial_port_base::baud_rate( MSR_BUAD_RATE ));
    this->port->set_option(serial_port_base::stop_bits( MSR_STOP_BITS ));
    this->port->set_option(serial_port_base::character_size( MSR_WORD_LENGHT ));
}

MSRDevice::~MSRDevice()
{
    delete port;
}

std::string MSRDevice::getSerialNumber()
{
    size_t response_size = 8;
    uint8_t command[] = {0x81, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(command, sizeof(command), response, response_size);

    uint64_t serial = (response[3] << 16) + (response[2] << 8) + response[1];
    delete response;
    return std::to_string(serial);
}

struct tm MSRDevice::getTime()
{
    size_t response_size = 8;
    uint8_t command[] = {0x8C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(command, sizeof(command), response, response_size);

    struct tm msr_time;
    msr_time.tm_year = response[6] + 100; //response[6] contains years since 2000
    msr_time.tm_mon = response[5];
    msr_time.tm_mday = response[4] + 1;
    msr_time.tm_hour = response[3];
    msr_time.tm_min = response[2];
    msr_time.tm_sec = response[1];
    msr_time.tm_isdst = -1;
    mktime(&msr_time);
    delete response;
    return msr_time;
}

std::string MSRDevice::getName()
{
    //first, collect the first 6 chars of the namespace
    std::string name;
    size_t response_size = 8;
    uint8_t command_first[] = {0x83, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t command_second[] = {0x83, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00};

    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(command_first, sizeof(command_first), response, response_size);

    name.append((const char *)response + 1, 6);
    this->sendcommand(command_second, sizeof(command_second), response, response_size);

    name.append((const char *)response + 1, 6);
    delete response;
    return name;

}

void MSRDevice::setName(std::string name)
{
    //if name is shorter than 12 characters (lenght of name), append spaces
    //This is also done by the proprietary driver
    while(name.size() < 12)
        name.push_back(' ');

    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];

    //this is some kind of "setup command" without it, the device won't accept the next commands
    uint8_t command_1[] = {0x85, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00};

    //these commands sets 4 characters each
    uint8_t command_2[] = {0x84, 0x05, 0x00, (uint8_t)name[0], (uint8_t)name[1], (uint8_t)name[2], (uint8_t)name[3]};
    uint8_t command_3[] = {0x84, 0x05, 0x01, (uint8_t)name[4], (uint8_t)name[5], (uint8_t)name[6], (uint8_t)name[7]};
    uint8_t command_4[] = {0x84, 0x05, 0x02, (uint8_t)name[8], (uint8_t)name[9], (uint8_t)name[10], (uint8_t)name[11]};

    this->sendcommand(command_1, sizeof(command_1), response, response_size);
    this->sendcommand(command_2, sizeof(command_2), response, response_size);
    this->sendcommand(command_3, sizeof(command_3), response, response_size);
    this->sendcommand(command_4, sizeof(command_4), response, response_size);
    delete response;
}

std::vector<rec_entry> MSRDevice::getRecordings()
{
    std::vector<rec_entry> rec_addresses;
    uint8_t first_placement_get[] = {0x82, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(first_placement_get, sizeof(first_placement_get), response, response_size);
    //the address to the first recording is placed in byte 4 and 5 these are least significant first.
    uint16_t address = (response[4] << 8) + response[3];

    //for the rest of the responses, the size of the response is 10 bytes, so we reallocate response
    response_size = 10;
    delete[] response;
    response = new uint8_t[10];

    //this command fetches the data at the adress given by byte 4 and 5
    uint8_t next_placement_get[] = {0x8B, 0x00, 0x00, (uint8_t)(address & 0xFF), (uint8_t)(address >> 8), 0x08, 0x00};
    bool next_new_entry = true;
    while(address != 0x1FFF)
    {
        this->sendcommand(next_placement_get, sizeof(next_placement_get), response, response_size);
        //adress to next entry is placed in byte 8 and 9

        uint16_t new_address = (response[8] << 8) + response[7];
        if(new_address == 0xFFFF) break; //if next adress it this, we are done
        if(next_new_entry == true)
        {
            next_new_entry = false;
            rec_entry new_entry;
            new_entry.address = address;
            //the timestamp of the entry is given in seconds since jan 1 2000
            //they are saved in the folowing parts of the response, ordered with msb first, lsb last
            //byte 3, byte 7, byte 6, byte 5(first 7 bits)
            int32_t entry_time_seconds = ((int32_t)response[2]) << 23;
            entry_time_seconds += ((int32_t)response[6]) << 15;
            entry_time_seconds += ((int32_t)response[5]) << 7;
            entry_time_seconds += ((int32_t)response[4]) >> 1;
            //setup the tm struct
            new_entry.time.tm_year = 100; //the msr count from 2000, not 1900
            new_entry.time.tm_mon = 0;
            new_entry.time.tm_mday = 1;
            new_entry.time.tm_hour = 0;
            new_entry.time.tm_min = 0;
            new_entry.time.tm_sec = entry_time_seconds;
            new_entry.time.tm_isdst = -1;
            mktime(&(new_entry.time));
            rec_addresses.push_back(new_entry);
        }

        if(new_address == address)
        {
            //if next adress is the same as requested
            //it means that we should count current adress down by one
            //and record the next entry
            next_new_entry = true;
            //set lenght of the entry
            uint16_t end_addr = rec_addresses.back().address;
            uint16_t start_addr = address;
            rec_addresses.back().lenght = end_addr - start_addr + 1;
            rec_addresses.back().address = start_addr; //adjust adress so it contains the start adress.

            if(--new_address == 0xFFFF)
            { //if adress underflows, it should underflow to 0x1FFF, which is the highest mem location on the MSR145
                new_address = 0x1FFF;
            }
        }

        address = new_address;
        next_placement_get[3] = address & 0xFF;
        next_placement_get[4] = address >> 8;
    }
    delete[] response;
    return rec_addresses;
}

std::vector<uint8_t> MSRDevice::readRecording(rec_entry record)
{ //recordings are read from the smallest memory location to the largest
    std::vector<uint8_t> recordData;
    size_t response_size = 0x0422;
    uint8_t *response = new uint8_t[response_size];

    //the fetch command. Format is:
    //0x8B 0x00 0x00 <address lsb> <address msb> <lenght lsb> <lenght msb>
    uint8_t fetch_command[] = {0x8B, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04};

    for(uint16_t i = 0; i < record.lenght; i++)
    {
        //send the fetch command
        uint16_t cur_addr = record.address + i;
        fetch_command[3] = cur_addr & 0xFF;
        fetch_command[4] = cur_addr >> 8;
        this->sendcommand(fetch_command, sizeof(fetch_command), response, response_size);

        //load the data into the vector. ignore first 9 bytes(for now), they are timestamp, etc
        if(i == 0)
        { //in the first chunk, the first 6 * 16 bytes are some kind of preample, which counts from 0 to 0xF
          //I don't really know what it means yet
            for(uint16_t j = 9 + 6 * 0xF + 8 ; j < response_size; j++)
                recordData.push_back(response[j]);
        }
        else
        {
            for(uint16_t j = 18; j < response_size; j++)
                recordData.push_back(response[j]);
        }
        //each sample is 32 bytes, and consists of  8 individual samples of 4 bytes each
        //first byte is an id, next 3 are the data
        //identified id's:
        //60 -- realative hydration
        //02 -- T_RH ?? divide by 256 to get the val in C.
        //10 -- pressure divide by 10 to get val in millibar
        //70 -- temp. something weird is happening in msb byte.


    }
    return recordData;
}
