cmake_minimum_required(VERSION 3.18)

project(raisin)
find_package(SDL2 REQUIRED)

add_executable(test src/test.cpp)
target_link_libraries(test SDL2::Main)
set_target_properties(test PROPERTIES
                      CXX_STANDARD 20 CXX_STANDARD_REQUIRED TRUE)
