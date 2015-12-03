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

std::string MSR_Reader::get_serial()
{
    size_t response_size = 8;
    uint8_t command[] = {0x81, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *response = new uint8_t[response_size];
    this->send_command(command, sizeof(command), response, response_size);

    uint64_t serial = (response[3] << 16) + (response[2] << 8) + response[1];
    delete[] response;
    return std::to_string(serial);
}

void MSR_Reader::convert_to_tm(uint8_t *response_ptr, struct tm *time_s)
{
    time_s->tm_year = response_ptr[6] + 100; //response[6] contains years since 2000
    time_s->tm_mon = response_ptr[5];
    time_s->tm_mday = response_ptr[4] + 1;
    time_s->tm_hour = response_ptr[3];
    time_s->tm_min = response_ptr[2];
    time_s->tm_sec = response_ptr[1];
    time_s->tm_isdst = -1;
    mktime(time_s);
}

struct tm MSR_Reader::get_time(uint8_t *command, uint8_t command_length)
{   //Returns the time returned by the command in "command"
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->send_command(command, command_length, response, response_size);
    //for(uint8_t i = 0; i < 7; i++) printf("%02X ", response[i]);
    //printf("\n");
    struct tm time_s;
    convert_to_tm(response, &time_s);
    delete[] response;
    return time_s;
}

struct tm MSR_Reader::get_device_time()
{
    uint8_t command[] = {0x8C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return get_time(command, sizeof(command));
}

rec_entry MSR_Reader::create_rec_entry(uint8_t *response_ptr, uint16_t start_addr, uint16_t end_addr, bool active)
{
    rec_entry entry;
    entry.address = start_addr;
    entry.length = end_addr - start_addr + 1;
    //the timestamp of the entry is given in seconds since jan 1 2000
    //they are saved in the folowing parts of the response, ordered with msb first, lsb last
    //byte 3, byte 7, byte 6, byte 5(first 7 bits)
    int32_t entry_time_seconds = ((int32_t)response_ptr[2]) << 23;
    entry_time_seconds += ((int32_t)response_ptr[6]) << 15;
    entry_time_seconds += ((int32_t)response_ptr[5]) << 7;
    entry_time_seconds += ((int32_t)response_ptr[4]) >> 1;
    //setup the tm struct
    entry.time.tm_year = 100; //the msr count from 2000, not 1900
    entry.time.tm_mon = 0;
    entry.time.tm_mday = 1;
    entry.time.tm_hour = 0;
    entry.time.tm_min = 0;
    entry.time.tm_sec = entry_time_seconds;
    entry.time.tm_isdst = -1;
    mktime(&(entry.time));
    entry.isRecording = active;
    return entry;
}
std::vector<rec_entry> MSR_Reader::get_rec_list(size_t max_num)
{
    std::vector<rec_entry> rec_addresses;
    uint8_t first_placement_get[] = {0x82, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->send_command(first_placement_get, sizeof(first_placement_get), response, response_size);
    //the address to the first recording is placed in byte 4 and 5 these are least significant first.
    uint16_t end_address = (response[4] << 8) + response[3]; //end address of the current entry
    uint16_t cur_address = end_address; //the adress we are going to request in the next command
    uint16_t start_address = end_address;
    uint8_t next_placement_get[] = {0x8B, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00};

    bool recording_active = (response[1] & 0x03); //this byte defines if the device is currently recording
    response_size = 10;
    delete[] response;
    response = new uint8_t[10];

    if(recording_active)
    {
        uint8_t active_recording_get[] = {0x8B, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00};
        this->send_command(active_recording_get, sizeof(active_recording_get), response, response_size);
        //for(uint8_t i = 0; i < 9; i++) printf("%02X ", response[i]);
        //printf("\n");
        cur_address = (response[8] <<  8) + response[7];
        next_placement_get[3] = cur_address & 0xFF;
        next_placement_get[4] = cur_address >> 8;
        if(response[1] == 0x01)
        {
            this->send_command(next_placement_get, sizeof(next_placement_get), response, response_size);
            //for(uint8_t i = 0; i < 9; i++) printf("%02X ", response[i]);
            //printf("\n");
        }
        start_address = (response[8] <<  8) + response[7];
        if(++end_address == 0x2000)
            end_address = 0x0000;
        rec_entry first_entry = create_rec_entry(response, start_address, end_address, true);
        rec_addresses.push_back(first_entry);
        start_address--;
        if(start_address == 0xFFFF)
        { //if adress underflows, it should underflow to 0x1FFF, which is the highest mem location on the MSR145
            start_address = 0x1FFF;
        }
        cur_address = start_address;
        end_address = start_address;
    }
    //for the rest of the responses, the size of the response is 10 bytes, so we reallocate response
    rec_entry new_entry;
    //this command fetches the data at the adress given by byte 4 and 5
    next_placement_get[3] = cur_address & 0xFF;
    next_placement_get[4] = cur_address >> 8;
    while(cur_address != 0x1FFF && (rec_addresses.size() < max_num || max_num == 0))
    {
        this->send_command(next_placement_get, sizeof(next_placement_get), response, response_size);
        //for(uint8_t i = 0; i < 9; i++) printf("%02X ", response[i]);
        //printf("\n");
        //adress to next entry is placed in byte 8 and 9
        switch(response[1])
        {
            case 0x01: //this means that what we requested was not the first page of the entry
                start_address = (response[8] <<  8) + response[7]; //these bytes hold the adress of the first page in the entry
                cur_address = start_address;
                break;
            case 0x21: //this means that what we requested was the first page of the entry
                //save the entry
                new_entry = create_rec_entry(response, start_address, end_address, false);
                rec_addresses.push_back(new_entry);
                //the next entrys end adress will be this ones start minus 1
                end_address = start_address - 1;
                if(end_address == 0xFFFF)
                { //if adress underflows, it should underflow to 0x1FFF, which is the highest mem location on the MSR145
                    end_address = 0x1FFF;
                }
                start_address = end_address; //set start adress as the same as end, so we don't fuckup on entries of length 1.
                cur_address = end_address; //request next entrys end adress next
                break;
            case 0xFF:
                return rec_addresses;
            default:
                printf("ERROR!!!!!\n"); //this should never happend
                break;
        }

        next_placement_get[3] = cur_address & 0xFF;
        next_placement_get[4] = cur_address >> 8;
    }
    delete[] response;
    return rec_addresses;
}

std::vector<uint8_t> MSR_Reader::get_raw_recording(rec_entry record)
{ //recordings are read from the smallest memory location to the largest
    if(!is_recording()) record.isRecording = false; //if we are not recording, this field is forced to be false.
    std::vector<uint8_t> recordData;
    size_t response_size = 0x0422;
    uint8_t *response = new uint8_t[response_size];

    //the fetch command. Format is:
    //0x8B 0x00 0x00 <address lsb> <address msb> <length lsb> <length msb>
    uint8_t fetch_command[] = {0x8B, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04};
    //std::vector<uint8_t> page_recordData;
    bool end = false;
    this->set_baud(230400);
    for(uint16_t i = 0; (i < record.length || record.isRecording) && !end; i++)
    {
        //send the fetch command
        uint16_t cur_addr = record.address + i;
        fetch_command[3] = cur_addr & 0xFF;
        fetch_command[4] = cur_addr >> 8;
        this->send_command(fetch_command, sizeof(fetch_command), response, response_size);

        //load the data into the vector. ignore first 9 bytes(for now), they are timestamp, etc
        uint16_t start_pos;
        if(i == 0)
        { //in the first chunk, the first 6 * 16 bytes are some kind of preample, which counts from 0 to 0xF
          //I don't really know what it means yet
          start_pos = 0 +9 + 6 * 0xF + 2;
        }
        else
        {
            start_pos = 17;
        }
        for(uint16_t j = start_pos; j < response_size - 1; j += 4)
        {
            if(response[j] == 0xFF && response[j + 1] == 0xFF && response[j + 2] == 0xFF && response[j + 3] == 0xFF)
            {
                end = true;
                if(record.isRecording && j == start_pos)  //this means that we are trying to access the current page.
                {
                    get_live_data(cur_addr, &recordData, i == 0);  //Fetch the data by other means.
                    //printf("%d\n\n\n", j);
                }
                break;
            }
            for(uint8_t k = 0; k < 4; k++)
                recordData.push_back(response[j + k]);
        }
    }
    this->set_baud(9600);
    delete[] response;
    return recordData;
}

void MSR_Reader::get_live_data(uint16_t cur_addr,std::vector<uint8_t> *recording_data, bool isFirstPage)
{   //Beware, this may contain bugs! Hard to debug, as the timing may be different each time.

    //first, fetch the data from the "live" page, using the special command.
    uint8_t start_pos = 17;
    if(isFirstPage) start_pos = 0 +9 + 6 * 0xF + 2;
    size_t response_size = 0x0422;
    uint8_t *live_data = new uint8_t[response_size];
    uint8_t *page_data = new uint8_t[response_size];
    //std::vector<uint8_t> data;
    uint8_t get_live_length[] = {0x82, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->send_command(get_live_length, sizeof(get_live_length), live_data, 8);
    uint16_t length = ((live_data[6] << 8) + live_data[5]) * 2;

    uint8_t get_live_page[] =   {0x8B, 0x00, 0x01, 0x00, 0x00, ((uint8_t)(length & 0xFF)), ((uint8_t)(length >> 8))};
    this->send_command(get_live_page, sizeof(get_live_page), live_data, length + 2);
    //check if the current address page is still unaccessable
    uint8_t fetch_command[] = {0x8B, 0x00, 0x00, ((uint8_t)(cur_addr & 0xFF)), ((uint8_t)(cur_addr >> 8)), 0x20, 0x04};
    this->send_command(fetch_command, sizeof(fetch_command), page_data, response_size);
    if(!(page_data[start_pos + 0] == 0xFF && page_data[start_pos + 1] == 0xFF && page_data[start_pos + 2] == 0xFF && page_data[start_pos + 3] == 0xFF))
    {
        //the page have changed. first, fetch new live page
        this->send_command(get_live_length, sizeof(get_live_length), live_data, 8);
        length = ((live_data[6] << 8) + live_data[5]) * 2;
        get_live_page[5] = length & 0xFF;
        get_live_page[6] = length >> 8;
        this->send_command(get_live_page, sizeof(get_live_page), live_data, length + 2);
        //read page into vector
        for(uint16_t i = start_pos; i < response_size - 1; i += 4)
        {
            if(page_data[i] == 0xFF && page_data[i + 1] == 0xFF && page_data[i + 2] == 0xFF && page_data[i + 3] == 0xFF)
                break;
            for(uint8_t j = 0; j < 4; j++)
            {
                recording_data->push_back(page_data[i + j]);
                //data.push_back(page_data[i + j]);
            }
        }
        //startpos will be 17 now
        start_pos = 17;
    }
    //read livedata into vector
    for(uint16_t i = start_pos; i < length - 1; i += 4)
    {
        if(live_data[i] == 0xFF && live_data[i + 1] == 0xFF && live_data[i + 2] == 0xFF && live_data[i + 3] == 0xFF)
            break;
        for(uint8_t j = 0; j < 4; j++)
        {
            recording_data->push_back(live_data[i + j]);
            //data.push_back(live_data[i + j]);

        }
    }
    //for(size_t i = 0; i < data.size(); i+=4) printf("%02X %02X %02X %02X\n", data[i], data[i + 1], data[i +2], data[i + 3]);
    delete[] live_data;
    delete[] page_data;
}

std::vector<sample> MSR_Reader::get_samples(rec_entry record)
{
    std::vector<sample> samples;
    auto rawdata = this->get_raw_recording(record);
    uint64_t timestamp = 0; //time since start of record in 1/512 seconds
    //run through the raw data and convert it to samples
    for(size_t i = 0; i < rawdata.size(); i += 4)
    {
        auto cur_sample = convert_to_sample(rawdata.data() + i, &timestamp);
        if(cur_sample.type == sampletype::end) break;
        if(cur_sample.type == sampletype::timestamp) continue;
        samples.push_back(cur_sample);
    }
    return samples;
}
sample MSR_Reader::convert_to_sample(uint8_t *sample_ptr, uint64_t *total_time)
{   //convert the 4 bytes pointed to by sample_ptr into the sample struct
    //the last 3 bytes are the sample data. however, not all types use all the bytes
    //some part of it is used for something else.
    sample this_sample;
    this_sample.type = (sampletype)(sample_ptr[1] >> 4);
    this_sample.rawsample = (sample_ptr[0] << 24) +(sample_ptr[1] << 16) + (sample_ptr[2] << 8) + sample_ptr[3];
    //check for end
    if(sample_ptr[0] == 0xFF && sample_ptr[1] == 0xFF && sample_ptr[2] == 0xFF && sample_ptr[3] == 0xFF)
        this_sample.type = sampletype::end;


    switch(this_sample.type)
    {
        case sampletype::T_pressure:
        case sampletype::pressure:
        case sampletype::T_humidity:
        case sampletype::humidity:
        case sampletype::bat:
        case sampletype::ext1:
        case sampletype::ext2:
        case sampletype::ext3:
        case sampletype::ext4:
        {
            //capture the sample value
            this_sample.value = (sample_ptr[3] << 8) + sample_ptr[2];
            //grab time change
            //time is the four lowest bits of byte 2, and all of byte 1
            uint16_t time_bits = (sample_ptr[1] & 0x0F);
            time_bits <<= 8;
            time_bits += sample_ptr[0];
            //the 5'th (highest) bit is not actually a part of the time.
            //it's a flag. If set, the time is in seconds, else it's in 1/512 seconds.
            //The last 11 bits is a signed int.
            if(time_bits & 0x0800)
                *total_time += ( (int16_t)((time_bits & 0x07FF) << 5) / 32 ) * (512);
            else
                *total_time += ( (int16_t)((time_bits & 0x07FF) << 5) / 32 );
            break;
        }
        case sampletype::timestamp:
        {
            //this is a special type, which is used when the timediff can't fit into the normal sample
            //the time is held in byte 1, 3 and 4, and is in 1/2 seconds.
            uint32_t timediff = (sample_ptr[0] << 16) + (sample_ptr[3] << 8) + sample_ptr[2];
            timediff *= 256; //the unit of total_time is 1/512 seconds
            *total_time += timediff;
            break;
        }
        default:
            printf("Unknown type: %02X\n", this_sample.type);
            this_sample.value = (sample_ptr[3] << 8) + sample_ptr[2];
            break;
        case end:
            break;
    }
    this_sample.timestamp = *total_time;

    return this_sample;
}

std::vector<uint16_t> MSR_Reader::get_sensor_data(std::vector<sampletype> &types)
{
    std::vector<uint16_t> return_vec;
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    for(size_t i = 0; i < types.size(); i += 3)
    {
        uint8_t typebytes[3] = {0x00, 0x00, 0x00};
        for(uint8_t j = 0; j < 3; j++)
            if(i + j < types.size() ) typebytes[j] = types[i + j];
        uint8_t fetch_data[] = {0x82, 0x02, typebytes[0], typebytes[1], typebytes[2], 0x00, 0x00};
        this->send_command(fetch_data, sizeof(fetch_data), response, response_size);

        for(uint8_t j = 0; j < 3; j++)
            if(return_vec.size() < types.size()) return_vec.push_back(response[j * 2 + 1] + (response[j * 2 + 2] << 8));
    }
    delete[] response;
    return return_vec;
}

std::string MSR_Reader::get_name()
{
    //first, collect the first 6 chars of the namespace
    std::string name;
    size_t response_size = 8;
    uint8_t command_first[] = {0x83, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t command_second[] = {0x83, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00};

    uint8_t *response = new uint8_t[response_size];
    this->send_command(command_first, sizeof(command_first), response, response_size);

    name.append((const char *)response + 1, 6);
    this->send_command(command_second, sizeof(command_second), response, response_size);

    name.append((const char *)response + 1, 6);
    delete[] response;
    return name;
}

std::string MSR_Reader::get_calibration_name()
{
    //first, collect the first 6 chars of the namespace
    std::string name;
    size_t response_size = 8;
    uint8_t command_first[] = {0x83, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00};
    uint8_t command_second[] = {0x83, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00};

    uint8_t *response = new uint8_t[response_size];
    this->send_command(command_first, sizeof(command_first), response, response_size);

    name.append((const char *)response + 5, 2);
    this->send_command(command_second, sizeof(command_second), response, response_size);

    name.append((const char *)response + 1, 6);
    delete[] response;
    return name;
}


uint32_t MSR_Reader::get_timer_interval(timer t)
{
    uint8_t *response = new uint8_t[8];
    uint8_t get_cmd[] = {0x83, 0x01, (uint8_t)t, 0x00, 0x00, 0x00, 0x00};
    this->send_command(get_cmd, sizeof(get_cmd), response, 8);
    uint32_t interval = (response[6] << 24) + (response[5] << 16) +
        (response[4] << 8) + response[3];
    delete[] response;
    return interval;
}

void MSR_Reader::get_active_measurements(timer t, uint8_t *measurements, bool *blink)
{   //Reads which measurements are active for this timer.
    //Saves the result in the given measurement and blink pointers.
    //It is done this way as we both need to determine if blink is active,
    //and the rest of the measurements
    uint8_t *response = new uint8_t[8];
    uint8_t get_cmd[] = {0x83, 0x00, (uint8_t)t, 0x00, 0x00, 0x00, 0x00};
    this->send_command(get_cmd, sizeof(get_cmd), response, 8);
    //for(uint8_t i = 0; i < 8; i++) printf("%02X ", response[i]);
    //it seems that some measurement types is also defined in byte 4, but i don't fully understand yet.
    *measurements =  response[5];
    *blink = response[6];
    delete[] response;
}

void MSR_Reader::get_start_setting(bool *bufferon, startcondition *start)
{
    uint8_t get_cmd[] = {0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->send_command(get_cmd, sizeof(get_cmd), response, response_size);
    //for(uint8_t i = 0; i < 8; i++) printf("%02X ", response[i]);
    //printf("\n");
    *bufferon = !(response[4] & 0x01);
    switch(response[2])
    {
        case 0x00: //This sometimes happens
            *start = startcondition::now;
            break;
        case 0x01:
            if(response[3] == 0x02) *start = startcondition::time_stop;
            else                    *start = startcondition::now;
            break;
        case 0x02:
            if(response[3] == 0x02) *start = startcondition::time_start_stop;
            else                    *start = startcondition::time_start;
            break;
        case 0x03:
            if(response[3] == 0x03) *start = startcondition::push_start_stop;
            else                    *start = startcondition::push_start;
            break;
    }
    delete[] response;
}
uint16_t MSR_Reader::get_general_lim_settings()
{   //the sampletypes where limits is active can be recieved by doing (returnval & (1 << sampletype))
    uint8_t get_cmd[] ={0x88, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->send_command(get_cmd, sizeof(get_cmd), response, response_size);
    uint16_t rec_limits = (response[4] << 8) + response[3];
    uint16_t alarm_limits = (response[6] << 8) + response[5];
    //for(uint8_t i = 0; i < 7; i++) printf("%02X ", response[i]);
    //printf("\n");
    delete[] response;
    //return the limits OR'd together. We don't care if it's an alarm-limit or rec.
    //this can be checked by a later command, which also give the actual limits.
    return rec_limits | alarm_limits;
}

void MSR_Reader::get_sample_lim_setting(sampletype type, uint8_t *rec_settings, uint8_t *alarm_settings, uint16_t *limit1, uint16_t *limit2)
{
    uint8_t get_cmd[] = {0x88, 0x0A, (uint8_t)type, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->send_command(get_cmd, sizeof(get_cmd), response, response_size);
    *rec_settings = response[1] & 0x07;
    *alarm_settings = response[1] & (0x07 << 3);
    *limit1 = (response[4] << 8) + response[3];
    *limit2 = (response[6] << 8) + response[5];
}

struct tm MSR_Reader::get_start_time()
{   //Get the time where sampling starts
    uint8_t command[] = {0x8C, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    return get_time(command, sizeof(command));
}

struct tm MSR_Reader::get_end_time()
{   //Get the time where sampling stops
    uint8_t command[] = {0x8C, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
    return get_time(command, sizeof(command));
}

void MSR_Reader::get_marker_setting(bool *marker_on, bool *alarm_confirm_on)
{
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    uint8_t get_cmd[] =  {0x88, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
    this->send_command(get_cmd, sizeof(get_cmd), response, response_size);
    //for(uint8_t i = 0; i < 7; i++) printf("%02X ", response[i]);
    //printf("\n");
    *marker_on = response[2];
    *alarm_confirm_on = response[3];
    delete[] response;
}
void MSR_Reader::get_calibrationdata(sampletype type, uint16_t *point_1_target, uint16_t *point_1_actual,
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
    delete[] response;
}
