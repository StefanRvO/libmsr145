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

//sends the given command to the MSR145 and read out_length number of bytes from it into out
uint8_t MSR_Base::calcChecksum(uint8_t *data, size_t length)
{
    //The MSR145 uses the dallas 8-bit CRC as checksum. Here we use boost to calculate it.
    boost::crc_optimal<8, 0x31, 0x00, 0x00, true, true> crc;
    crc.process_bytes(data, length);
    return crc.checksum();
}
void MSR_Base::sendcommand(uint8_t *command, size_t command_length,
                            uint8_t *out, size_t out_length)
{
    bool selfalloced = false;
    if(out == nullptr && out_length > 0)
    {
        selfalloced = true;
        out = new uint8_t[out_length];
    }

    auto checksum = calcChecksum(command, command_length);
    do
    {
        for(size_t i = 0; i < command_length; i++) printf("%02X ", command[i]); printf("\n");
        write(*(this->port), buffer(command, command_length));
        write(*(this->port), buffer(&checksum, sizeof(checksum)));
        size_t read_bytes = read(*(this->port), buffer(out, out_length), transfer_exactly(out_length));
        assert(read_bytes == out_length);
        assert(out_length == 0 || (out[out_length - 1] == calcChecksum(out, out_length - 1)));
        for(size_t i = 0; i < out_length; i++) printf("%02X ", out[i]); printf("\n\n");
    } while(out_length && (out[0] & 0x20) ); // if response have 0x20 set, it means error (normaly because it didn't have time to respond).
    if(selfalloced) delete[] out;
}

void MSR_Base::sendraw(uint8_t *command, size_t command_length,
                            uint8_t *out, size_t out_length)
{
    bool selfalloced = false;
    if(out == nullptr)
    {
        selfalloced = true;
        out = new uint8_t[out_length];
    }
    write(*(this->port), buffer(command, command_length));
    size_t read_bytes = read(*(this->port), buffer(out, out_length), transfer_exactly(out_length));
    assert(read_bytes == out_length);
    assert(out_length == 0 || (out[out_length - 1] == calcChecksum(out, out_length - 1)));
    if(selfalloced) delete[] out;
}


MSR_Base::MSR_Base(std::string _portname)
{
    //open the port and setup baudrate, stop bits and word length
    this->portname = _portname;
    this->port = new serial_port(ioservice, _portname);
    this->port->set_option(serial_port_base::baud_rate( MSR_BUAD_RATE ));
    this->port->set_option(serial_port_base::stop_bits( MSR_STOP_BITS ));
    this->port->set_option(serial_port_base::character_size( MSR_WORD_length ));
    //read_timer = new deadline_timer(this->ioservice);

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
    //0x8B 0x00 0x00 <address lsb> <address msb> <length lsb> <length msb>
    uint8_t command[] = {0x86, 0x03, 0x00, 0xFF, 0x00, 0x00, 0x00};
    this->sendcommand(command, sizeof(command), nullptr, 8);
    usleep(20000); //needed to prevent staaling when doing many succesive calls
}

void MSR_Base::format_memory()
{
    stopRecording(); //stop current recording before format.
    uint16_t start_address = 0x0000;
    uint16_t end_address = 0x03FF;
    uint8_t erase_cmd[] = {0x8A, 0x06, 0x00,
        0x00 /*adress lsb*/, 0x00 /*address msb*/,
        0x5A, 0xA5};
    uint8_t start_cmd1[] = {0x8A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->sendcommand(start_cmd1, sizeof(start_cmd1), nullptr, 8);

    uint8_t confirm_command[] = {0x8A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *returnval = new uint8_t[8];
    for(uint16_t addr = start_address; addr <= end_address; addr++)
    {
        erase_cmd[3] = addr & 0xFF;
        erase_cmd[4] = addr >> 8;
        this->sendcommand(erase_cmd, sizeof(erase_cmd), returnval, 8);
        do
        {
            this->sendcommand(confirm_command, sizeof(confirm_command), returnval, 8);
        } while(returnval[1] != 0xBC);
    }
    delete[] returnval;
}

void MSR_Base::stopRecording()
{
    if(!(this->isRecording())) return;
    uint8_t stop_command[] = {0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->sendcommand(stop_command, sizeof(stop_command), nullptr, 8);
}

bool MSR_Base::isRecording()
{
    uint8_t first_placement_get[] = {0x82, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(first_placement_get, sizeof(first_placement_get), response, response_size);
    bool recording_active = (response[1] == 0x83); //this byte defines if the device is currently recording
    delete[] response;
    return recording_active;
}
