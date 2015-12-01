#
# Test CMake version
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

set (MSR145TOOL_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (MSR145TOOL_HEADERDIR "${MSR145TOOL_DIR}/headers")
set (MSR145TOOL_HEADERS ${MSR145TOOL_HEADERDIR}/msr145_tool.hpp)


include_directories(${MSR145TOOL_HEADERDIR} "${ROOT}/libmsr145/headers/")

# And now we add any targets that we want
add_subdirectory(sources)
