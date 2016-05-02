CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)

add_executable(msr145_tool msr145_tool.cpp main.cpp options_handler.cpp ${MSR145TOOL_HEADERS})
add_executable(msr145_com msr145_tool.cpp msr145_com.cpp options_handler.cpp ${MSR145TOOL_HEADERS})
target_link_libraries (msr145_tool msr145 boost_program_options)
target_link_libraries (msr145_com msr145 boost_program_options)
