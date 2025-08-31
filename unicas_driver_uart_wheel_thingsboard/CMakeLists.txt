cmake_minimum_required(VERSION 3.10)

# Set some basic project attributes
project (unicas_driver_uart_wheel_thingsboard
	VERSION 0.1
	DESCRIPTION "A Hello World Project")


set(CMAKE_CXX_STANDARD 17)

# Trova la libreria mosquitto e gli header
find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
find_library(MOSQUITTO_LIBRARY NAMES mosquitto)

if (NOT MOSQUITTO_INCLUDE_DIR OR NOT MOSQUITTO_LIBRARY)
    message(FATAL_ERROR "libmosquitto not found. Install it (e.g. libmosquitto-dev)")
endif()

include_directories(${MOSQUITTO_INCLUDE_DIR})

# This project will output an executable file
add_executable(${PROJECT_NAME} unicas_driver_uart_wheel_thingsboard.cpp SerialDevice.h SerialUNO.h SerialNANO.h UDPReceiver.h MQTTClient.h can.h VirtualKeyboard.h)

target_link_libraries(${PROJECT_NAME} ${MOSQUITTO_LIBRARY})

# Include the configuration header in the build
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")
