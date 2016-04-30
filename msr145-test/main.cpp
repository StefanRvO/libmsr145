#include "libmsr145.hpp"
#include <iostream>
#include <bitset>
int main()
{
    MSRDevice msr("/dev/ttyUSB0");
    float gain, offset;
    msr.get_L1_offset_gain(&offset, &gain);
    std::cout << gain << "\t" << offset << std::endl;
}
