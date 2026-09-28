cmake_minimum_required(VERSION 3.20)

project(FF8R-Analog360
        VERSION 1.0.0
        LANGUAGES CXX
)

set(DLL_OUTPUT_NAME dinput8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_EXTENSIONS OFF)

add_library(${DLL_OUTPUT_NAME} SHARED squall360.cpp)
set_target_properties(${DLL_OUTPUT_NAME} PROPERTIES PREFIX "")
