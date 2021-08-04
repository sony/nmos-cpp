cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# project name
project(nmos-cpp)

# enable or disable the example applications
set(BUILD_EXAMPLES ON CACHE BOOL "Build example applications")

# enable or disable the unit test suite
set(BUILD_TESTS ON CACHE BOOL "Build test suite application")

# enable or disable the LLDP support library (lldp)
# and its additional dependencies
set(BUILD_LLDP OFF CACHE BOOL "Build LLDP support library")
mark_as_advanced(FORCE BUILD_LLDP)

# common config
include(cmake/NmosCppCommon.cmake)

# nmos-cpp dependencies
include(cmake/NmosCppDependencies.cmake)

# nmos-cpp libraries
include(cmake/NmosCppLibraries.cmake)

if(BUILD_EXAMPLES)
    # nmos-cpp-node executable
    include(cmake/NmosCppNode.cmake)

    # nmos-cpp-registry executable
    include(cmake/NmosCppRegistry.cmake)
endif()

if(BUILD_TESTS)
    # nmos-cpp-test executable
    include(cmake/NmosCppTest.cmake)
endif()

# export the config-file package
include(cmake/NmosCppExports.cmake)
