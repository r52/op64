cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(op64)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(FATAL_ERROR "This CMakeLists.txt is not configured for non-Linux machines! Please Use the Visual Studio solution on windows")
endif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Installing to ${CMAKE_SOURCE_DIR}/bin" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_subdirectory(op64-util)
add_subdirectory(op64core)
add_subdirectory(op64-qt)
