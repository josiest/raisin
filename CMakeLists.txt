cmake_minimum_required(VERSION 3.18)
project(raisin VERSION 0.1.0)
add_library(raisin INTERFACE)

find_package(range-v3 REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(tl-expected REQUIRED)
 
target_link_libraries(raisin INTERFACE
    range-v3::range-v3
    tomlplusplus::tomlplusplus
    tl::expected)

#
# Export and install the library
#

include(GNUInstallDirs)

# define the include directory for the library
target_include_directories(raisin INTERFACE
    $<BUILD_INTERFACE:${raisin_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# define the export targets
install(TARGETS raisin
        EXPORT raisin-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# install the include files
install(DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")

add_library(raisin::raisin INTERFACE IMPORTED)
set_target_properties(raisin::raisin PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/src")

#
# Export and install the package
#

include(CMakePackageConfigHelpers)

# create the config settings
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/raisin-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/raisin-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raisin)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/raisin-config-version.cmake
    COMPATIBILITY AnyNewerVersion)

# install the config settings
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/raisin-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/raisin-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raisin)

# export and install the cmake targets for the library
install(EXPORT raisin-targets
        FILE raisin-targets.cmake
        NAMESPACE raisin::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raisin)
