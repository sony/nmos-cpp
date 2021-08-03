cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# project name
project(nmos-cpp)

# common config
include(cmake/NmosCppCommon.cmake)

# enable or disable the LLDP support library (lldp)
# and its additional dependencies
set(BUILD_LLDP OFF CACHE BOOL "Build LLDP support library")

# nmos-cpp dependencies
include(cmake/NmosCppDependencies.cmake)

# nmos-cpp libraries
include(cmake/NmosCppLibraries.cmake)

# nmos-cpp-node executable
include(cmake/NmosCppNode.cmake)

# nmos-cpp-registry executable
include(cmake/NmosCppRegistry.cmake)

# nmos-cpp-test executable
include(cmake/NmosCppTest.cmake)
