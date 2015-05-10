cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(op64-util CXX)

set(UTIL_HEADERS 
        md5.h
        oplib.h
        oplog.h
        oppreproc.h
        opstrutil.h
        optime.h
        oputil.h)

set(UTIL_SOURCES
        md5.cpp
        oplib.cpp
        oplog.cpp
        oppreproc.cpp
        optime.cpp)

add_library(op64-util STATIC ${UTIL_SOURCES})

set_property(TARGET op64-util PROPERTY CXX_STANDARD 11)
set_property(TARGET op64-util PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(op64-util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
