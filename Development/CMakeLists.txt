# CMake 3.9 is required due to cpprestsdk-config.cmake using find_dependency with COMPONENTS
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# project name
project(nmos-cpp)

# The default nmos-cpp root directory
set(NMOS_CPP_DIR ${PROJECT_SOURCE_DIR})

set (USE_CONAN ON CACHE BOOL "Use Conan to acquire dependencies")

if (${USE_CONAN})
    include (${NMOS_CPP_DIR}/cmake/NmosCppConan.cmake)
endif()

# Common setup and dependency checking
include (${NMOS_CPP_DIR}/cmake/NmosCppCommon.cmake)

# Setup for the libraries
include (${NMOS_CPP_DIR}/cmake/NmosCppLibraries.cmake)

# nmos-cpp-node executable

set(NMOS_CPP_NODE_SOURCES
    ${NMOS_CPP_DIR}/nmos-cpp-node/main.cpp
    ${NMOS_CPP_DIR}/nmos-cpp-node/node_implementation.cpp
    )
set(NMOS_CPP_NODE_HEADERS
    ${NMOS_CPP_DIR}/nmos-cpp-node/node_implementation.h
    )

add_executable(
    nmos-cpp-node
    ${NMOS_CPP_NODE_SOURCES}
    ${NMOS_CPP_NODE_HEADERS}
    ${NMOS_CPP_DIR}/nmos-cpp-node/config.json
    )

source_group("Source Files" FILES ${NMOS_CPP_NODE_SOURCES})
source_group("Header Files" FILES ${NMOS_CPP_NODE_HEADERS})

target_link_libraries(
    nmos-cpp-node
    nmos-cpp_static
    ${CPPRESTSDK_TARGET}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Conan packages usually don't include PDB files so suppress the resulting warning
    set_target_properties(
        nmos-cpp-node
        PROPERTIES
        LINK_FLAGS "/ignore:4099"
        )
endif()

# nmos-cpp-registry executable

set(NMOS_CPP_REGISTRY_SOURCES
    ${NMOS_CPP_DIR}/nmos-cpp-registry/main.cpp
    ${NMOS_CPP_DIR}/nmos-cpp-registry/registry_implementation.cpp
    )
set(NMOS_CPP_REGISTRY_HEADERS
    ${NMOS_CPP_DIR}/nmos-cpp-registry/registry_implementation.h
    )

add_executable(
    nmos-cpp-registry
    ${NMOS_CPP_REGISTRY_SOURCES}
    ${NMOS_CPP_REGISTRY_HEADERS}
    ${NMOS_CPP_DIR}/nmos-cpp-registry/config.json
    )

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_SOURCES})
source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_HEADERS})

target_link_libraries(
    nmos-cpp-registry
    nmos-cpp_static
    ${CPPRESTSDK_TARGET}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Conan packages usually don't include PDB files so suppress the resulting warning
    set_target_properties(
        nmos-cpp-registry
        PROPERTIES
        LINK_FLAGS "/ignore:4099"
        )
endif()

# nmos-cpp-test executable
include (${NMOS_CPP_DIR}/cmake/NmosCppTest.cmake)
