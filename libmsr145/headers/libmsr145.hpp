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
#include "libmsr145_enums.hpp"
#include "libmsr145_structs.hpp"

#define MSR_BUAD_RATE 9600
#define MSR_STOP_BITS boost::asio::serial_port_base::stop_bits::one
#define MSR_WORD_length 8


class MSR_Base
{
    protected:
        boost::asio::serial_port *port;
        boost::asio::io_service ioservice;
        std::string portname;
    public:
        MSR_Base(std::string _portname);
        virtual ~MSR_Base();
        virtual void set_baud(uint32_t baudrate);
        virtual void update_sensors(); //not really sure which class to put this in.
        virtual void format_memory();
        virtual void stop_recording();
        virtual bool is_recording();
    public: //protected:
        virtual int send_command(uint8_t *command, size_t command_length, uint8_t *out, size_t out_length);
        virtual void send_raw(uint8_t * command, size_t command_length, uint8_t *out, size_t out_length);
        virtual uint8_t calc_chksum(uint8_t *data, size_t length);


};

class MSR_Writer : virtual public MSR_Base
{
    public:
        MSR_Writer(std::string _portname) : MSR_Base(_portname) {};
        virtual void set_names_and_calibration_date(std::string deviceName, std::string calibrationName,
            uint8_t year, uint8_t month, uint8_t day);

        virtual void set_time(struct tm *timeset = nullptr);
        virtual void start_recording(startcondition start_set,
            struct tm *starttime = nullptr, struct tm *stoptime = nullptr, bool ringbuffer = false);
        virtual void set_timer_interval(timer t, uint64_t interval);
        virtual void set_timer_measurements(timer t, uint8_t bitmask = 0x00, bool blink = 0);
        virtual void set_limit(sampletype type, uint16_t limit1, uint16_t limit2,
            limit_setting record_limit, limit_setting alarm_limit);
        virtual void reset_limits();
        virtual void set_marker_settings(bool marker_on, bool alarm_confirm_on);
        virtual void set_calibrationdata(sampletype type, uint16_t point_1_target, uint16_t point_1_actual,
            uint16_t point_2_target, uint16_t point_2_actual);
        virtual void insert_time_in_command(struct tm *timeset, uint8_t *command);
};

class MSR_Reader : virtual public MSR_Base
{
    public:
        MSR_Reader(std::string _portname) : MSR_Base(_portname) {};
        virtual std::string get_serial();
        virtual std::string get_name();
        virtual std::string get_calibration_name();
        virtual struct tm get_time(uint8_t *command, uint8_t command_length);
        virtual struct tm get_device_time();
        virtual struct tm get_start_time();
        virtual struct tm get_end_time();
        virtual std::vector<rec_entry> get_rec_list(); //only work when recording is not active
        virtual std::vector<sample> get_samples(rec_entry record);
        virtual std::vector<uint16_t> get_sensor_data(std::vector<sampletype> &types);
        virtual uint32_t get_timer_interval(timer t);
        virtual void get_active_measurements(timer t, uint8_t *measurements, bool *blink);
        virtual void get_start_setting(bool *bufferon, startcondition *start);
        virtual uint16_t get_general_lim_settings();
        virtual void get_sample_lim_setting(sampletype type, uint8_t *limit_setting, uint16_t *limit1, uint16_t *limit2);
        virtual void convert_to_tm(uint8_t *response_ptr, struct tm * time_s);
        virtual void get_marker_setting(bool *marker_on, bool *alarm_confirm_on);
        virtual void get_live_data(uint16_t cur_addr, std::vector<uint8_t> *recording_data, bool isFirstPage);
        virtual void get_calibrationdata(sampletype type, uint16_t *point_1_target, uint16_t *point_1_actual,
            uint16_t *point_2_target, uint16_t *point_2_actual);

    protected:
        virtual std::vector<uint8_t> get_raw_recording(rec_entry record);
        virtual sample convert_to_sample(uint8_t *sample_ptr, uint64_t *total_time);
        virtual rec_entry create_rec_entry(uint8_t *response_ptr, uint16_t start_addr, uint16_t end_addr, bool active);
};

class MSRDevice : public MSR_Writer, public MSR_Reader
{
    public:
        MSRDevice(std::string _portname) :
        MSR_Base(_portname), MSR_Writer(_portname), MSR_Reader(_portname) {};
};
