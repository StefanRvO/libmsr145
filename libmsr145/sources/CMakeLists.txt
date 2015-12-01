#
# Test CMake version
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)



# And now we add any targets that we want
add_library(msr145 libmsr145_base.cpp libmsr145_reader.cpp libmsr145_writer.cpp ${LIBMSR145_HEADERS})
target_link_libraries(msr145 boost_system)




