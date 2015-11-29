#pragma once
#include <cstdint>
#include "libmsr145_enums.hpp"
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
