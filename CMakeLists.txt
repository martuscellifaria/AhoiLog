cmake_minimum_required(VERSION 3.28)
project(VHLog)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set (CMAKE_SYSTEM_VERSION 10.0)
    file(GLOB all_SRCS
            "${PROJECT_SOURCE_DIR}/include/*.h"
            "${PROJECT_SOURCE_DIR}/src/*.cpp"
            "${PROJECT_SOURCE_DIR}/examples/*.cpp"
            )
    add_executable(VHLog ${all_SRCS})
    set_property(TARGET VHLog PROPERTY CXX_STANDARD 23)
    target_include_directories(VHLog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    file(GLOB all_SRCS
            "${PROJECT_SOURCE_DIR}/include/*.h"
            "${PROJECT_SOURCE_DIR}/src/*.cpp"
            "${PROJECT_SOURCE_DIR}/examples/*.cpp"
    )

    add_executable(VHLog ${all_SRCS})
    target_include_directories(VHLog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_property(TARGET VHLog PROPERTY CXX_STANDARD 23)
endif()

