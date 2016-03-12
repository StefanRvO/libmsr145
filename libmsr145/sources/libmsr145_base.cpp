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
#include <chrono>
#include <thread> //sleep_for
using namespace boost::asio;

//sends the given command to the MSR145 and read out_length number of bytes from it into out
uint8_t MSR_Base::calc_chksum(uint8_t *data, size_t length)
{
    //The MSR145 uses the dallas 8-bit CRC as checksum. Here we use boost to calculate it.
    boost::crc_optimal<8, 0x31, 0x00, 0x00, true, true> crc;
    crc.process_bytes(data, length);
    return crc.checksum();
}
int MSR_Base::send_command(uint8_t *command, size_t command_length,
                            uint8_t *out, size_t out_length)
    //Returns 0 on success
{
    int returncode = 0;
    bool selfalloced = false;
    if(out == nullptr && out_length > 0)
    {
        selfalloced = true;
        out = new uint8_t[out_length];
    }
    auto checksum = calc_chksum(command, command_length);
    //    for(size_t i = 0; i < command_length; i++) printf("%02X ", command[i]); printf("\n");
        write(*(this->port), buffer(command, command_length));
        write(*(this->port), buffer(&checksum, sizeof(checksum)));
        size_t read_bytes = read(*(this->port), buffer(out, out_length), transfer_exactly(out_length));
        assert(read_bytes == out_length);
        assert(out_length == 0 || (out[out_length - 1] == calc_chksum(out, out_length - 1)));
        //for(size_t i = 0; i < out_length; i++) printf("%02X ", out[i]); printf("\n\n");
        //std::this_thread::sleep_for(std::chrono::milliseconds(20)); //We need to sleep a bit after changeing baud, else we will stall
    if(out_length && (out[0] & 0x20) ) returncode = 1; // if response have 0x20 set, it means error (normaly because it didn't have time to respond).
    if(selfalloced) delete[] out;
    return returncode;
}

void MSR_Base::send_raw(uint8_t *command, size_t command_length,
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
    assert(out_length == 0 || (out[out_length - 1] == calc_chksum(out, out_length - 1)));
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
    this->port->set_option(serial_port::flow_control(serial_port::flow_control::none));
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
    this->send_command(command, sizeof(command), nullptr, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); //We need to sleep a bit after changeing baud, else we will stall
    this->port->set_option(serial_port_base::baud_rate( baudrate ));
}

void MSR_Base::update_sensors()
{
    //the fetch command. Format is:
    //0x8B 0x00 0x00 <address lsb> <address msb> <length lsb> <length msb>
    uint8_t command[] = {0x86, 0x03, 0x00, 0xFF, 0x00, 0x00, 0x00};
    this->send_command(command, sizeof(command), nullptr, 8);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void MSR_Base::format_memory()
{
    stop_recording(); //stop current recording before format.
    uint16_t start_address = 0x0000;
    uint16_t end_address = 0x03FF;
    uint8_t erase_cmd[] = {0x8A, 0x06, 0x00,
        0x00 /*adress lsb*/, 0x00 /*address msb*/,
        0x5A, 0xA5};
    uint8_t start_cmd1[] = {0x8A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->send_command(start_cmd1, sizeof(start_cmd1), nullptr, 8);

    uint8_t confirm_command[] = {0x8A, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *returnval = new uint8_t[8];
    for(uint16_t addr = start_address; addr <= end_address; addr++)
    {
        erase_cmd[3] = addr & 0xFF;
        erase_cmd[4] = addr >> 8;
        bool success = false;
        while(!success)
            if(this->send_command(erase_cmd, sizeof(erase_cmd), returnval, 8) == 0)
                success = true;
        do
        {
            this->send_command(confirm_command, sizeof(confirm_command), returnval, 8);
        } while(returnval[1] != 0xBC);
    }
    delete[] returnval;
}

void MSR_Base::stop_recording()
{
    if(!(this->is_recording())) return;
    uint8_t stop_command[] = {0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->send_command(stop_command, sizeof(stop_command), nullptr, 8);
}

bool MSR_Base::is_recording()
{
    uint8_t first_placement_get[] = {0x82, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->send_command(first_placement_get, sizeof(first_placement_get), response, response_size);
    bool recording_active = (response[1] & 0x03); //this byte defines if the device is currently recording
    delete[] response;
    return recording_active;
}

void MSR_Base::get_calibrationdata(calibration_type::calibration_type type, uint16_t *point_1_target, uint16_t *point_1_actual,
    uint16_t *point_2_target, uint16_t *point_2_actual)
{
    uint8_t *response = new uint8_t[8];
    uint8_t getpoint1[] = {0x88, 0x0C, (uint8_t)type, 0x00, 0x00, 0x00, 0x00};
    uint8_t getpoint2[] = {0x88, 0x0D, (uint8_t)type, 0x00, 0x00, 0x00, 0x00};
    this->send_command(getpoint1, sizeof(getpoint1), response, 8);
    *point_1_target = (response[4] << 8) + response[3];
    *point_1_actual = (response[6] << 8) + response[5];
    this->send_command(getpoint2, sizeof(getpoint2), response, 8);
    *point_2_target = (response[4] << 8) + response[3];
    *point_2_actual = (response[6] << 8) + response[5];
    //printf("G%d\t%d\t%d\t%d\t%d\n", type, *point_1_target, *point_1_actual, *point_2_target, *point_2_actual);

    delete[] response;
}
