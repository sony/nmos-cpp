cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(my-nmos-node)

# This project actually uses the source files of nmos-cpp-node
set(NMOS_CPP_DIRECTORY "../../Development" CACHE STRING "Path to nmos-cpp/Development directory")

# Find and use nmos-cpp from its installed location or as a subdirectory
set(USE_ADD_SUBDIRECTORY OFF CACHE BOOL "Use add_subdirectory() rather than find_package()")
if (NOT USE_ADD_SUBDIRECTORY)
    find_package(nmos-cpp REQUIRED)
else()
    add_subdirectory(${NMOS_CPP_DIRECTORY} build-nmos-cpp EXCLUDE_FROM_ALL)
endif()

# my-nmos-node executable

set(MY_NMOS_NODE_SOURCES
    ${NMOS_CPP_DIRECTORY}/nmos-cpp-node/main.cpp
    ${NMOS_CPP_DIRECTORY}/nmos-cpp-node/node_implementation.cpp
    )
set(MY_NMOS_NODE_HEADERS
    ${NMOS_CPP_DIRECTORY}/nmos-cpp-node/node_implementation.h
    )

add_executable(
    my-nmos-node
    ${MY_NMOS_NODE_SOURCES}
    ${MY_NMOS_NODE_HEADERS}
    ${NMOS_CPP_DIRECTORY}/nmos-cpp-node/config.json
    )

source_group("Source Files" FILES ${MY_NMOS_NODE_SOURCES})
source_group("Header Files" FILES ${MY_NMOS_NODE_HEADERS})

target_link_libraries(
    my-nmos-node
    nmos-cpp::compile-settings
    nmos-cpp::nmos-cpp
    )
