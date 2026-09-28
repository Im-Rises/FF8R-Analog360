cmake_minimum_required(VERSION 3.20)

project(FF8R-Analog360
        VERSION 1.0.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_EXTENSIONS OFF)

add_library(dinput8 SHARED squall360.cpp)
