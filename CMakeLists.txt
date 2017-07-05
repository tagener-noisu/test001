cmake_minimum_required(VERSION 3.9.1)
project(test)
include(CTest)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -g -Wall -Wunused -pedantic")

include_directories(include)
file(GLOB SOURCES "src/*.c")

add_executable(app ${SOURCES})

find_library(LEV libev.so)
target_link_libraries(app ${LEV})
