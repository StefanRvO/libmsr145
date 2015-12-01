CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

add_executable(msr145_tool msr145_tool.cpp main.cpp ${MSR145TOOL_HEADERS})
target_link_libraries (msr145_tool msr145 boost_program_options)
