cmake_minimum_required(VERSION 3.13)
project(http-server-starter-cpp)

set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

set(SOURCE_FILES src/server.cpp)

add_executable(server ${SOURCE_FILES})

# Link the pthread library
target_link_libraries(server pthread)

# Find the zlib library
find_package(ZLIB REQUIRED)

# # Create the executable
# add_executable(server ${SOURCE_FILES})

# Link the zlib library
target_link_libraries(server ZLIB::ZLIB)
