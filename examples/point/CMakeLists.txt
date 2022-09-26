cmake_minimum_required(VERSION 3.18)
project(spawn-points-example)

add_executable(sketch sketch.cpp)
set_target_properties(sketch PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE)

find_package(datalang REQUIRED)
target_link_libraries(sketch INTERFACE datalang)
