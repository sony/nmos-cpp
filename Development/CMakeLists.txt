# the injection point may be used to configure conan, but that requires CMake 3.24 or higher
# so avoid confusion and reject invocations which attempt to use it on lower versions
# see https://cmake.org/cmake/help/v3.24/variable/CMAKE_PROJECT_TOP_LEVEL_INCLUDES.html
# the alternative is to run conan install first instead
if(CMAKE_PROJECT_TOP_LEVEL_INCLUDES)
    cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
endif()

# project name
project(nmos-cpp)

# enable or disable the example applications
set(NMOS_CPP_BUILD_EXAMPLES ON CACHE BOOL "Build example applications")

# enable or disable the unit test suite
set(NMOS_CPP_BUILD_TESTS ON CACHE BOOL "Build test suite application")

# enable or disable the LLDP support library (lldp)
# and its additional dependencies
set(NMOS_CPP_BUILD_LLDP OFF CACHE BOOL "Build LLDP support library")
mark_as_advanced(FORCE NMOS_CPP_BUILD_LLDP)

# common config
include(cmake/NmosCppCommon.cmake)

# nmos-cpp dependencies
include(cmake/NmosCppDependencies.cmake)

# nmos-cpp libraries
include(cmake/NmosCppLibraries.cmake)

if(NMOS_CPP_BUILD_EXAMPLES)
    # nmos-cpp-node executable
    include(cmake/NmosCppNode.cmake)

    # nmos-cpp-registry executable
    include(cmake/NmosCppRegistry.cmake)
endif()

if(NMOS_CPP_BUILD_TESTS)
    # nmos-cpp-test executable
    include(cmake/NmosCppTest.cmake)
endif()

# export the config-file package
include(cmake/NmosCppExports.cmake)
