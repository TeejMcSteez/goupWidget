cmake_minimum_required(VERSION 3.20)
project(TrayDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(traydemo main.cpp)
target_link_libraries(traydemo PRIVATE Qt6::Widgets)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
target_link_libraries(traydemo PRIVATE Qt6::Widgets Qt6::Network)
