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
#include <boost/crc.hpp>
#include <cstdio>
using namespace boost::asio;

//sends the given command to the MSR145 and read out_lenght number of bytes from it into out
uint8_t MSR_Base::calcChecksum(uint8_t *data, size_t lenght)
{
    //The MSR145 uses the dallas 8-bit CRC as checksum. Here we use boost to calculate it.
    boost::crc_optimal<8, 0x31, 0x00, 0x00, true, true> crc;
    crc.process_bytes(data, lenght);
    return crc.checksum();
}
void MSR_Base::sendcommand(uint8_t *command, size_t command_lenght,
                            uint8_t *out, size_t out_lenght)
{
    bool selfalloced = false;
    if(out == nullptr && out_lenght > 0)
    {
        selfalloced = true;
        out = new uint8_t[out_lenght];
    }
    auto checksum = calcChecksum(command, command_lenght);
    write(*(this->port), buffer(command, command_lenght));
    write(*(this->port), buffer(&checksum, sizeof(checksum)));
    size_t read_bytes = read(*(this->port), buffer(out, out_lenght), transfer_exactly(out_lenght));
    assert(read_bytes == out_lenght);
    assert(out_lenght == 0 || (out[out_lenght - 1] == calcChecksum(out, out_lenght - 1)));
    if(selfalloced) delete[] out;
}

void MSR_Base::sendraw(uint8_t *command, size_t command_lenght,
                            uint8_t *out, size_t out_lenght)
{
    bool selfalloced = false;
    if(out == nullptr)
    {
        selfalloced = true;
        out = new uint8_t[out_lenght];
    }
    write(*(this->port), buffer(command, command_lenght));
    size_t read_bytes = read(*(this->port), buffer(out, out_lenght), transfer_exactly(out_lenght));
    assert(read_bytes == out_lenght);
    assert(out_lenght == 0 || (out[out_lenght - 1] == calcChecksum(out, out_lenght - 1)));
    if(selfalloced) delete[] out;
}


MSR_Base::MSR_Base(std::string _portname)
{
    //open the port and setup baudrate, stop bits and word lenght
    this->portname = _portname;
    this->port = new serial_port(ioservice, _portname);
    this->port->set_option(serial_port_base::baud_rate( MSR_BUAD_RATE ));
    this->port->set_option(serial_port_base::stop_bits( MSR_STOP_BITS ));
    this->port->set_option(serial_port_base::character_size( MSR_WORD_LENGHT ));
}

MSR_Base::~MSR_Base()
{
    //set baud to 9600 so we can open quickly again
    set_baud(MSR_BUAD_RATE);
    delete port;
}

void MSR_Base::set_baud(uint32_t baudrate)
{
    uint8_t baudbyte;
    switch(baudrate)
    {
        case 9600:
            baudbyte = 0x00;
            break;
        case 19200:
            baudbyte = 0x01;
            break;
        case 38400:
            baudbyte = 0x02;
            break;
        case 57600:
            baudbyte = 0x03;
            break;
        case 115200:
            baudbyte = 0x04;
            break;
        case 230400:
            baudbyte = 0x05;
            break;
        default:
            printf("%d is not a valid baudrate.\n", baudrate);
            return;

    }
    uint8_t command[] = {0x85, 0x01, baudbyte, 0x00, 0x00, 0x00, 0x00};
    this->sendcommand(command, sizeof(command), nullptr, 0);
    usleep(20000); //We need to sleep a bit after changeing baud, else we will stall
    this->port->set_option(serial_port_base::baud_rate( baudrate ));
}

void MSR_Base::updateSensors()
{
    //the fetch command. Format is:
    //0x8B 0x00 0x00 <address lsb> <address msb> <lenght lsb> <lenght msb>
    uint8_t command[] = {0x86, 0x03, 0x00, 0xFF, 0x00, 0x00, 0x00};
    this->sendcommand(command, sizeof(command), nullptr, 8);
    usleep(20000); //needed to prevent staaling when doing many succesive calls
}
