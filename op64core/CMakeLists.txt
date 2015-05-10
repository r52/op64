cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(op64core CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB_RECURSE CORE_SOURCES "./*.cpp")
file(GLOB_RECURSE CORE_HEADERS "./*.h")

add_library(op64core STATIC ${CORE_SOURCES})
target_link_libraries(op64core op64-util)

set_property(TARGET op64core PROPERTY CXX_STANDARD 11)
set_property(TARGET op64core PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(op64core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
