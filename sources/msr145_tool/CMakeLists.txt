#
# Test CMake version
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)



# And now we add any targets that we want
add_executable(msr145_tool main.cpp)
target_link_libraries (msr145_tool msr145 boost_program_options)
