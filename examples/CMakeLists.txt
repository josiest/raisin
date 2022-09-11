cmake_minimum_required(VERSION 3.18)
project(raisin-sketch)

add_executable(sketch sketch.cpp)
set_target_properties(sketch PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE)

find_package(SDL2 REQUIRED)
target_link_libraries(sketch PRIVATE SDL2::SDL2)

find_package(raisin REQUIRED)
target_link_libraries(sketch INTERFACE raisin::raisin)
