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
        boost::asio::deadline_timer *read_timer;
    public:
        MSR_Base(std::string _portname);
        virtual ~MSR_Base();
        virtual void set_baud(uint32_t baudrate);
        virtual bool is_recording();
        virtual std::string get_L1_unit_str();  //unfortunately, we need to place this here, as it's needed in the writer
        virtual void get_L1_offset_gain(float *offset, float *gain);  //unfortunately, we need to place this here, as it's needed in the writer
        virtual void get_calibrationdata(calibration_type::calibration_type type, uint16_t *point_1_target, uint16_t *point_1_actual,
            uint16_t *point_2_target, uint16_t *point_2_actual); //unfortunately, we need to place this here, as it's needed in the writer
        virtual int send_command(uint8_t *command, size_t command_length, uint8_t *out, size_t out_length);

    protected:
        virtual void send_raw(uint8_t * command, size_t command_length, uint8_t *out, size_t out_length);
        virtual uint8_t calc_chksum(uint8_t *data, size_t length);
        virtual int send_with_timeout(uint8_t *command, size_t command_length,
                                    uint8_t *out, size_t out_length, boost::posix_time::time_duration time_out);


};

class MSR_Writer : virtual public MSR_Base
{
    public:
        MSR_Writer(std::string _portname) : MSR_Base(_portname) {};
        virtual void format_memory();
        virtual void stop_recording();

        virtual int set_names_and_calibration_date(std::string deviceName, std::string calibrationName,
            uint8_t year, uint8_t month, uint8_t day, uint8_t active_calib);

        virtual int set_time(struct tm *timeset = nullptr);
        virtual int start_recording(startcondition start_set,
            struct tm *starttime = nullptr, struct tm *stoptime = nullptr, bool ringbuffer = false);
        virtual int set_timer_interval(uint8_t t, uint64_t interval);
        virtual int set_timer_measurements(uint8_t t, uint8_t bitmask = 0x00, bool blink = 0, bool active = true);
        virtual int set_limit(sampletype type, uint16_t limit1, uint16_t limit2,
            limit_setting record_limit, limit_setting alarm_limit);
        virtual int reset_limits();
        virtual int set_marker_settings(bool marker_on, bool alarm_confirm_on);
        virtual int set_calibrationdata(calibration_type::calibration_type type, uint16_t point_1_target, uint16_t point_1_actual,
            uint16_t point_2_target, uint16_t point_2_actual);
        virtual int set_L1_unit(std::string unit);
        virtual int set_L1_offset_gain(float offset, float gain);
    protected:
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
        virtual void update_sensors(); //not really sure which class to put this in.
        virtual std::vector<rec_entry> get_rec_list(size_t max_num = 0);
        virtual std::vector<sample> get_samples(rec_entry record);
        virtual std::vector<int16_t> get_sensor_data(std::vector<sampletype> &types);
        virtual uint32_t get_timer_interval(uint8_t t);
        virtual void get_active_measurements(uint8_t t, uint8_t *measurements, bool *blink);
        virtual void get_start_setting(bool *bufferon, startcondition *start);
        virtual uint16_t get_general_lim_settings();
        virtual void get_sample_lim_setting(sampletype type, uint8_t *rec_settings,
            uint8_t *alarm_settings, uint16_t *limit1, uint16_t *limit2);
        virtual void convert_to_tm(uint8_t *response_ptr, struct tm * time_s);
        virtual void get_marker_setting(bool *marker_on, bool *alarm_confirm_on);
        virtual std::string get_calibration_name(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *active_calib);
        virtual void get_firmware_version(int *major, int *minor);
    private:
        virtual std::vector<std::pair<std::vector<uint8_t>, uint64_t> > get_raw_recording(rec_entry record);
        virtual sample convert_to_sample(uint8_t *sample_ptr, uint64_t *total_time);
        virtual rec_entry create_rec_entry(uint8_t *response_ptr, uint16_t start_addr, uint16_t end_addr, bool active);
        virtual uint64_t get_page_timestamp(uint8_t *response);
        virtual void get_live_data(std::vector<std::pair<std::vector<uint8_t>, uint64_t> > &sample_pages,
            uint16_t cur_addr, bool isFirstPage);
        virtual void add_raw_samples(std::vector<std::pair<std::vector<uint8_t>, uint64_t> > &sample_pages,
            bool &end, uint8_t *response, size_t response_size, uint16_t start_pos, bool live, uint16_t page_num, uint16_t cur_addr);

};

class MSRDevice : public MSR_Writer, public MSR_Reader
{
    public:
        MSRDevice(std::string _portname) :
        MSR_Base(_portname), MSR_Writer(_portname), MSR_Reader(_portname) {};
};
