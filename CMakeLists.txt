cmake_minimum_required(VERSION 3.9)
project(receivePhoneData)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g")

add_executable(receivePhoneData main.cpp headers/TCPServer.hpp headers/Thread.hpp headers/Utils.hpp headers/Constants.hpp src/TCPServer.cpp src/Thread.cpp src/Utils.cpp)