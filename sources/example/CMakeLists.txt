#
# Test CMake version
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)



# And now we add any targets that we want
add_executable(msr145_test main.cpp ${HEADER_FILES})
target_link_libraries (msr145_test msr145)
