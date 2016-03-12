#
# Test CMake version
#
CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)



# And now we add any targets that we want
add_executable(msr145_test main.cpp)
include_directories("${ROOT}/libmsr145/headers")
target_link_libraries (msr145_test msr145)
