cmake_minimum_required(VERSION 3.10)
project(QuadGNSS_Sim VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define PI if not defined
add_definitions(-DM_PI=3.14159265358979323846)

# QuadGNSS Interface Library
set(QUAD_GNSS_HEADERS
    include/quad_gnss_interface.h
)

# Source files for the interface
set(QUAD_GNSS_SOURCES
    src/quad_gnss_test.cpp
)

# Create interface test executable
add_executable(interface_test
    src/interface_test.cpp
    ${QUAD_GNSS_SOURCES}
)

# Link math library
if(UNIX)
    target_link_libraries(interface_test m)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(interface_test PRIVATE /W4)
else()
    target_compile_options(interface_test PRIVATE -Wall -Wextra -pedantic)
endif()