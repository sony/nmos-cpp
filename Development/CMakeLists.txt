cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

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
