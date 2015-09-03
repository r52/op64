cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(op64-util CXX)

# Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.59 COMPONENTS chrono date_time filesystem log system thread REQUIRED)

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

add_library(op64-util SHARED ${UTIL_SOURCES})
target_link_libraries(op64-util ${Boost_LIBRARIES})

set_property(TARGET op64-util PROPERTY CXX_STANDARD 14)
set_property(TARGET op64-util PROPERTY CXX_STANDARD_REQUIRED ON)

set_target_properties (op64-util PROPERTIES DEFINE_SYMBOL "OP_DLL_EXPORTS" )

target_include_directories(op64-util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

install(TARGETS op64-util
        RUNTIME DESTINATION bin COMPONENT Runtime
        LIBRARY DESTINATION bin COMPONENT Runtime
        )