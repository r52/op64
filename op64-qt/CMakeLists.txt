cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(op64-qt CXX)

# Qt defs
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Libs

# Qt
find_package(Qt5Widgets REQUIRED)

# Boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.59 COMPONENTS chrono date_time filesystem log system thread REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

get_target_property(QT_LIB_LOC Qt5::Widgets LOCATION)
get_filename_component(QT_LIB_DIR ${QT_LIB_LOC} DIRECTORY)

message(STATUS "Qt Lib directory:       " ${QT_LIB_DIR})

# Sources
file(GLOB UI_HEADERS "*.h")
file(GLOB UI_SOURCES "*.cpp")

add_executable(op64-qt ${UI_SOURCES} qop64window.qrc)

# C++14
set_property(TARGET op64-qt PROPERTY CXX_STANDARD 14)
set_property(TARGET op64-qt PROPERTY CXX_STANDARD_REQUIRED ON)

# Linking
target_link_libraries(op64-qt op64core op64-util)
target_link_libraries(op64-qt ${Boost_LIBRARIES})
#target_link_libraries(op64-qt ${QT_LIB_DIR}/../plugins/platforms/libqxcb.a)
#target_link_libraries(op64-qt ${QT_LIB_DIR}/libQt5XcbQpa.a)
#target_link_libraries(op64-qt ${QT_LIB_DIR}/libxcb-static.a)
#target_link_libraries(op64-qt ${QT_LIB_DIR}/libQt5PlatformSupport.a)
target_link_libraries(op64-qt Qt5::Widgets Qt5::Gui Qt5::Core)
#target_link_libraries(op64-qt ${QT_LIB_DIR}/libqtharfbuzzng.a)
#target_link_libraries(op64-qt ${QT_LIB_DIR}/libqtpcre.a)
target_link_libraries(op64-qt ${OPENGL_LIBRARIES})
#target_link_libraries(op64-qt dl X11-xcb xcb)
#target_link_libraries(op64-qt freetype fontconfig Xi)

install(TARGETS op64-qt DESTINATION bin)
