cmake_minimum_required(VERSION 3.18)

project(raisin)
find_package(SDL2 REQUIRED)
find_package(range-v3 REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(tl-expected REQUIRED)

add_executable(test src/test.cpp)
target_link_libraries(test PUBLIC SDL2::SDL2)
target_link_libraries(test PRIVATE range-v3::range-v3)
target_link_libraries(test INTERFACE tomlplusplus::tomlplusplus
                                     tl-expected::tl-expected)

set_target_properties(test PROPERTIES
                      CXX_STANDARD 20 CXX_STANDARD_REQUIRED TRUE)

target_include_directories(test PUBLIC include)
target_compile_options(test PRIVATE -fconcepts-diagnostics-depth=2)
