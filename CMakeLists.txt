cmake_minimum_required(VERSION 3.20)

project(FF8R-Analog360
        VERSION 1.0.0
        LANGUAGES CXX
)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
            "Game is made in x86, it needs the .dll as x86. Compile in x86!\n"
            "Switch to a x86 compiler."
    )
endif()

set(DLL_OUTPUT_NAME dinput8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_EXTENSIONS OFF)

if(MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
endif()

add_library(${DLL_OUTPUT_NAME} SHARED dinput8Proxy.cpp dinput8.def)

if (MINGW)
    target_link_options(${DLL_OUTPUT_NAME} PRIVATE -static -Wl,--enable-stdcall-fixup)
endif ()

set_target_properties(${DLL_OUTPUT_NAME} PROPERTIES PREFIX "")

#target_link_libraries(${DLL_OUTPUT_NAME} PRIVATE xinput9_1_0)
