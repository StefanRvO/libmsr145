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

std::string MSR_Reader::getSerialNumber()
{
    size_t response_size = 8;
    uint8_t command[] = {0x81, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(command, sizeof(command), response, response_size);

    uint64_t serial = (response[3] << 16) + (response[2] << 8) + response[1];
    delete[] response;
    return std::to_string(serial);
}

struct tm MSR_Reader::getTime()
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
    delete[] response;
    return msr_time;
}

rec_entry MSR_Reader::create_rec_entry(uint8_t *response_ptr, uint16_t start_addr, uint16_t end_addr)
{
    rec_entry entry;
    entry.address = start_addr;
    entry.lenght = end_addr - start_addr + 1;
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
    return entry;
}
std::vector<rec_entry> MSR_Reader::getRecordinglist()
{
    std::vector<rec_entry> rec_addresses;
    uint8_t first_placement_get[] = {0x82, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    this->sendcommand(first_placement_get, sizeof(first_placement_get), response, response_size);
    //the address to the first recording is placed in byte 4 and 5 these are least significant first.
    uint16_t end_address = (response[4] << 8) + response[3]; //end address of the current entry
    uint16_t cur_address = end_address; //the adress we are going to request in the next command
    uint16_t start_address = end_address;
    uint8_t next_placement_get[] = {0x8B, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00};

    bool recording_active = (response[1] == 0x83); //this byte defines if the device is currently recording
    response_size = 10;
    delete[] response;
    response = new uint8_t[10];

    if(recording_active)
    {
        uint8_t active_recording_get[] = {0x8B, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00};
        this->sendcommand(active_recording_get, sizeof(active_recording_get), response, response_size);
        //for(uint8_t i = 0; i < 9; i++) printf("%02X ", response[i]);
        //printf("\n");
        cur_address = (response[8] <<  8) + response[7];
        next_placement_get[3] = cur_address & 0xFF;
        next_placement_get[4] = cur_address >> 8;
        if(response[1] == 0x01)
        {
            this->sendcommand(next_placement_get, sizeof(next_placement_get), response, response_size);
            //for(uint8_t i = 0; i < 9; i++) printf("%02X ", response[i]);
            //printf("\n");
        }
        start_address = (response[8] <<  8) + response[7];
        if(++end_address == 0x2000)
            end_address = 0x0000;
        rec_entry first_entry = create_rec_entry(response, start_address, end_address);
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
    while(cur_address != 0x1FFF)
    {
        this->sendcommand(next_placement_get, sizeof(next_placement_get), response, response_size);
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
                new_entry = create_rec_entry(response, start_address, end_address);
                rec_addresses.push_back(new_entry);
                //the next entrys end adress will be this ones start minus 1
                end_address = start_address - 1;
                if(end_address == 0xFFFF)
                { //if adress underflows, it should underflow to 0x1FFF, which is the highest mem location on the MSR145
                    end_address = 0x1FFF;
                }
                start_address = end_address; //set start adress as the same as end, so we don't fuckup on entries of lenght 1.
                cur_address = end_address; //request next entrys end adress next
                break;
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

std::vector<uint8_t> MSR_Reader::getRawRecording(rec_entry record)
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
            for(uint16_t j = 0 +9 + 6 * 0xF + 2 ; j < response_size - 1; j++)
            {
                recordData.push_back(response[j]);
            }
        }
        else
        {
            for(uint16_t j = 17; j < response_size - 1; j++)
                recordData.push_back(response[j]);
        }
    }
    delete[] response;
    return recordData;
}

std::vector<sample> MSR_Reader::getSamples(rec_entry record)
{
    std::vector<sample> samples;
    auto rawdata = this->getRawRecording(record);
    uint64_t timestamp = 0; //time since start of record in 1/512 seconds
    //run through the raw data and convert it to samples
    for(size_t i = 0; i < rawdata.size(); i += 4)
    {
        auto cur_sample = convertToSample(rawdata.data() + i, &timestamp);
        if(cur_sample.type == sampletype::end) break;
        samples.push_back(cur_sample);
    }
    return samples;
}
sample MSR_Reader::convertToSample(uint8_t *sample_ptr, uint64_t *total_time)
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
            //There is still something a bit wrong with this part.
            if(time_bits & 0x0800)
                *total_time += (time_bits & 0x07FF) * 512;
            else
                *total_time += (time_bits & 0x07FF);
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

void MSR_Reader::getSensorData(int16_t *returnvalues, sampletype type1, sampletype type2, sampletype type3)
{
    size_t response_size = 8;
    uint8_t *response = new uint8_t[response_size];
    uint8_t fetch_data[] = {0x82, 0x02, (uint8_t)type1, (uint8_t)type2, (uint8_t)type3, 0x00, 0x00};
    this->sendcommand(fetch_data, sizeof(fetch_data), response, response_size);
    if(type1 != sampletype::none) returnvalues[0] = response[1] + (response[2] << 8);
    if(type2 != sampletype::none) returnvalues[1] = response[3] + (response[4] << 8);
    if(type3 != sampletype::none) returnvalues[2] = response[5] + (response[6] << 8);
    usleep(20000); //needed to prevent staaling when doing many succesive calls
    delete[] response;
}

std::string MSR_Reader::getName()
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
    delete[] response;
    return name;

}
