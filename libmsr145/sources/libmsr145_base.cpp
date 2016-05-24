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
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
using namespace boost::asio;
using namespace boost::posix_time;


//sends the given command to the MSR145 and read out_length number of bytes from it into out
uint8_t MSR_Base::calc_chksum(uint8_t *data, size_t length)
{
    //The MSR145 uses the dallas 8-bit CRC as checksum. Here we use boost to calculate it.
    boost::crc_optimal<8, 0x31, 0x00, 0x00, true, true> crc;
    crc.process_bytes(data, length);
    return crc.checksum();
}

int MSR_Base::send_with_timeout(uint8_t *command, size_t command_length,
                            __attribute__((unused)) uint8_t *out, __attribute__((unused)) size_t out_length, time_duration time_out)
{
    //zero out output.
    memset(out, 0, out_length);
    auto checksum = calc_chksum(command, command_length);
    write(*(this->port), buffer(command, command_length));
    write(*(this->port), buffer(&checksum, sizeof(checksum)));
    boost::optional<boost::system::error_code> timer_result;
    boost::asio::deadline_timer timer(this->port->get_io_service());
    timer.expires_from_now(time_out);
    timer.async_wait([&timer_result] (const boost::system::error_code& error) { timer_result.reset(error); });

    boost::optional<boost::system::error_code> read_result;
    boost::asio::async_read(*(this->port), buffer(out, out_length), transfer_exactly(out_length), [&read_result] (const boost::system::error_code& error, size_t) { read_result.reset(error); });

    this->port->get_io_service().reset();
    bool success = false;
    while (this->port->get_io_service().run_one())
    {
        if (read_result)
        {
            timer.cancel();
            success = true;
        }
        else if (timer_result)
        {
            this->port->cancel();
            success = false;
        }
    }
    if(success == false)
        return 1;
    else if(out_length == 0) return 0;
    for(size_t i = 0; i < out_length; i++)
    {
        if(out[i] != 0) return 0;
    }
    return 1;
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
    //printf("SEND: ");    for(size_t i = 0; i < command_length; i++) printf("%02X ", command[i]); printf("\n");
    int error;
    do {
        error = send_with_timeout(command, command_length, out, out_length, time_duration(0,0,1,0));
    } while(error != 0);

    //printf("RECIEVE: ");    for(size_t i = 0; i < out_length; i++) printf("%02X ", out[i]); printf("\n\n");
    if(out_length && (out[0] & 0x20) ) returncode = 1; // if response hav   e 0x20 set, it means error (normaly because it didn't have time to respond).
    //if(out_length > 2 && (out[0] == 0x00) &&  ) assert(false); //this should not happen.
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
    //std::this_thread::sleep_for(std::chrono::milliseconds(20)); //We need to sleep a bit.

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


std::string MSR_Base::get_L1_unit_str()
{
    uint8_t get[] = {0x88, 0x03, 0x09, 0x00, 0x00, 0x00, 0x00};
    uint8_t *response = new uint8_t[8];
    std::string ret_str;
    this->send_command(get, sizeof(get), response, 8);
    for(int i = 3; i <= 6; i++)
        ret_str += response[i];
    delete[] response;
    return ret_str;
}

void MSR_Base::get_L1_offset_gain(float *offset, float *gain)
{
    //The gain and offset saved for L1 is a floating point numbers. It is a 24 bit floating point
    //Number. We pad it with 00 to make it compatible.
    uint8_t get_gain[] = {0x88, 0x04, 0x09, 0x00, 0x00, 0x00, 0x00};
    uint8_t *response = new uint8_t[8];
    this->send_command(get_gain, sizeof(get_gain), response, 8);
    //save the gain and offset
    //BEWARE THIS ONLY WORK ON LITTLE ENDIAN. NEEDS TO BE REVERSED FOR BIG ENDIAN!
    uint8_t *gain_ptr = ((uint8_t *)gain);
    uint8_t *offset_ptr =  ((uint8_t *)offset);
    gain_ptr[0] = 0x00; gain_ptr[1] = response[4]; gain_ptr[2] = response[5]; gain_ptr[3] = response[6];
    offset_ptr[0] = 0x00; offset_ptr[1] = response[1]; offset_ptr[2] = response[2]; offset_ptr[3] = response[3];
    delete[] response;
}
