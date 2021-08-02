# CMake 3.9 is required due to cpprestsdk-config.cmake using find_dependency with COMPONENTS
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# project name
project(nmos-cpp)

# The default nmos-cpp root directory
set(NMOS_CPP_DIR ${PROJECT_SOURCE_DIR})

# common config
include(${NMOS_CPP_DIR}/cmake/NmosCppCommon.cmake)

# enable or disable the LLDP support library (lldp)
# and its additional dependencies
set(BUILD_LLDP OFF CACHE BOOL "Build LLDP support library")

# nmos-cpp dependencies
include(${NMOS_CPP_DIR}/cmake/NmosCppDependencies.cmake)

# nmos-cpp libraries
include(${NMOS_CPP_DIR}/cmake/NmosCppLibraries.cmake)

# nmos-cpp-node executable
include(${NMOS_CPP_DIR}/cmake/NmosCppNode.cmake)

# nmos-cpp-registry executable
include(${NMOS_CPP_DIR}/cmake/NmosCppRegistry.cmake)

# nmos-cpp-test executable
include(${NMOS_CPP_DIR}/cmake/NmosCppTest.cmake)
