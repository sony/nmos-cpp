# CMake 3.9 is required due to cpprestsdk-config.cmake using find_dependency with COMPONENTS
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# project name
project(nmos-cpp)

# enable C++14
enable_language(CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# note: to see the output of any failed tests, set CTEST_OUTPUT_ON_FAILURE=1 in the environment
# and also remember that CMake doesn't add dependencies to the "test" (or "RUN_TESTS") target
# so after changing code under test, it is important to "make all" (or build "ALL_BUILD")
enable_testing()

# location of additional cmake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    )

# guard against in-source builds and bad build-type strings
include(safeguards)

# find cpprestsdk
# note: 2.10.0 minimum but there's no cpprestsdk-configVersion.cmake
# and CPPREST_VERSION_MAJOR, etc. also aren't exported by cpprestsdk::cpprest
find_package(cpprestsdk REQUIRED NAMES cpprestsdk cpprest)
if(cpprestsdk_FOUND)
    message(STATUS "Found cpprestsdk")
endif()
get_target_property(CPPREST_INCLUDE_DIR cpprestsdk::cpprest INTERFACE_INCLUDE_DIRECTORIES)

# find websocketpp
# note: good idea to use same version as cpprestsdk was built with!
if(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    message(STATUS "Using configured websocketpp include directory at " ${WEBSOCKETPP_INCLUDE_DIR})
else()
    find_package(websocketpp 0.5.1 REQUIRED)
    if(websocketpp_FOUND)
        message(STATUS "Found websocketpp version " ${websocketpp_VERSION})
        message(STATUS "Using websocketpp include directory at " ${WEBSOCKETPP_INCLUDE_DIR})
    endif()
endif()

# find boost
# note: 1.57.0 doesn't work due to https://svn.boost.org/trac10/ticket/10754
find_package(Boost 1.54.0 REQUIRED COMPONENTS system date_time)

# set platform-specific variables for dependencies
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # find bonjour (for mdns_static)
    # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR aren't set, the headers and library are assumed to be installed in the system paths
    set (BONJOUR_LIB libdns_sd.so)

    # add dependency required by nmos/filesystem_route.cpp
    set (PLATFORM_LIBS -lstdc++fs)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # find bonjour (for mdns_static)
    set (BONJOUR_INCLUDE "${PROJECT_SOURCE_DIR}/../../Bonjour SDK/Include")
    set (BONJOUR_LIB_DIR "${PROJECT_SOURCE_DIR}/../../Bonjour SDK/Lib/x64")
    set (BONJOUR_LIB dnssd)

    # force cpprestsdk to use pplx (for nmos-cpp-node and nmos-cpp-registry)
    # note: don't actually need this since target_link_libraries(cpprestsdk::cpprest) does this if it's been configured correctly
    add_definitions(/DCPPREST_FORCE_PPLX=1)

    # does one of our dependencies result in needing to do this?
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt")
endif()

# set common C++ compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif(MSVC)
    # set CharacterSet to Unicode rather than MultiByte
    add_definitions(/DUNICODE /D_UNICODE)
endif()

# set most compiler warnings on
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wstrict-aliasing -fstrict-aliasing -Wextra -Wno-unused-parameter -pedantic -Wno-long-long")
elseif(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"detail/vc_disable_warnings.h\"")
endif()

# location of header files (should be using specific target_include_directories?)
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CPPREST_INCLUDE_DIR} # defined above from target cpprestsdk::cpprest of find_package(cpprestsdk)
    ${WEBSOCKETPP_INCLUDE_DIR} # defined by find_package(websocketpp)
    ${Boost_INCLUDE_DIRS} # defined by find_package(Boost) 
    ${BONJOUR_INCLUDE} # defined above
    )

# location of libraries
link_directories(
    ${Boost_LIBRARY_DIRS}
    ${BONJOUR_LIB_DIR}
    )

# common preprocessor definitions
add_definitions(/DSLOG_STATIC /DSLOG_LOGGING_SEVERITY=slog::max_verbosity)

# mdns library

set(MDNS_SOURCES
    mdns/bonjour_dns_impl.cpp
    mdns/core.cpp
    mdns/service_advertiser.cpp
    mdns/service_discovery.cpp
    )
set(MDNS_HEADERS
    mdns/bonjour_dns_impl.h
    mdns/core.h
    mdns/service_advertiser.h
    mdns/service_discovery.h
    )

add_library(
    mdns_static STATIC
    ${MDNS_SOURCES}
    ${MDNS_HEADERS}
    )

source_group("Source Files" FILES ${MDNS_SOURCES})
source_group("Header Files" FILES ${MDNS_HEADERS})

# nmos-cpp-node

set(NMOS_CPP_NODE_SOURCES
    nmos-cpp-node/main.cpp
    )
set(NMOS_CPP_NODE_HEADERS
    nmos-cpp-node/main_gate.h
    )

set(NMOS_CPP_NODE_CPPREST_SOURCES
    cpprest/api_router.cpp
    cpprest/host_utils.cpp
    cpprest/http_utils.cpp
    cpprest/json_utils.cpp
    )
set(NMOS_CPP_NODE_CPPREST_HEADERS
    cpprest/api_router.h
    cpprest/basic_utils.h
    cpprest/host_utils.h
    cpprest/http_utils.h
    cpprest/json_utils.h
    cpprest/logging_utils.h
    cpprest/regex_utils.h
    )

set(NMOS_CPP_NODE_NMOS_SOURCES
    nmos/api_downgrade.cpp
    nmos/api_utils.cpp
    nmos/connection_api.cpp
    nmos/logging_api.cpp
    nmos/mdns.cpp
    nmos/node_api.cpp
    nmos/node_registration.cpp
    nmos/node_resources.cpp
    nmos/process_utils.cpp
    nmos/query_utils.cpp
    nmos/resources.cpp
    nmos/settings_api.cpp
    )
set(NMOS_CPP_NODE_NMOS_HEADERS
    nmos/activation_mode.h
    nmos/api_downgrade.h
    nmos/api_utils.h
    nmos/api_version.h
    nmos/connection_api.h
    nmos/health.h
    nmos/id.h
    nmos/json_fields.h
    nmos/log_manip.h
    nmos/logging_api.h
    nmos/mdns.h
    nmos/model.h
    nmos/node_api.h
    nmos/node_registration.h
    nmos/node_resources.h
    nmos/paging_utils.h
    nmos/process_utils.h
    nmos/query_utils.h
    nmos/rational.h
    nmos/resource.h
    nmos/resources.h
    nmos/settings.h
    nmos/settings_api.h
    nmos/slog.h
    nmos/tai.h
    nmos/thread_utils.h
    nmos/type.h
    nmos/version.h
    )

set(NMOS_CPP_NODE_RQL_SOURCES
    rql/rql.cpp
    )
set(NMOS_CPP_NODE_RQL_HEADERS
    rql/rql.h
    )

set(NMOS_CPP_NODE_SLOG_HEADERS
    slog/all_in_one.h
    )

add_executable(
    nmos-cpp-node
    ${NMOS_CPP_NODE_SOURCES}
    ${NMOS_CPP_NODE_HEADERS}
    ${NMOS_CPP_NODE_CPPREST_SOURCES}
    ${NMOS_CPP_NODE_CPPREST_HEADERS}
    ${NMOS_CPP_NODE_NMOS_SOURCES}
    ${NMOS_CPP_NODE_NMOS_HEADERS}
    ${NMOS_CPP_NODE_RQL_SOURCES}
    ${NMOS_CPP_NODE_RQL_HEADERS}
    ${NMOS_CPP_NODE_SLOG_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_NODE_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_NODE_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_NODE_NMOS_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_NODE_RQL_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_NODE_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_NODE_CPPREST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_NODE_NMOS_HEADERS})
source_group("rql\\Header Files" FILES ${NMOS_CPP_NODE_RQL_HEADERS})
source_group("slog\\Header Files" FILES ${NMOS_CPP_NODE_SLOG_HEADERS})

target_link_libraries(
    nmos-cpp-node
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

# nmos-cpp-registry

set(NMOS_CPP_REGISTRY_SOURCES
    nmos-cpp-registry/main.cpp
    )
set(NMOS_CPP_REGISTRY_HEADERS
    nmos-cpp-registry/main_gate.h
    )

set(NMOS_CPP_REGISTRY_CPPREST_SOURCES
    cpprest/api_router.cpp
    cpprest/host_utils.cpp
    cpprest/http_utils.cpp
    cpprest/json_utils.cpp
    cpprest/ws_listener_impl.cpp
    )
set(NMOS_CPP_REGISTRY_CPPREST_HEADERS
    cpprest/api_router.h
    cpprest/basic_utils.h
    cpprest/host_utils.h
    cpprest/http_utils.h
    cpprest/json_utils.h
    cpprest/logging_utils.h
    cpprest/regex_utils.h
    cpprest/ws_listener.h
    )

set(NMOS_CPP_REGISTRY_NMOS_SOURCES
    nmos/admin_ui.cpp
    nmos/api_downgrade.cpp
    nmos/api_utils.cpp
    nmos/filesystem_route.cpp
    nmos/logging_api.cpp
    nmos/mdns.cpp
    nmos/mdns_api.cpp
    nmos/node_api.cpp
    nmos/process_utils.cpp
    nmos/query_api.cpp
    nmos/query_utils.cpp
    nmos/query_ws_api.cpp
    nmos/registration_api.cpp
    nmos/resources.cpp
    nmos/server_resources.cpp
    nmos/settings_api.cpp
    )
set(NMOS_CPP_REGISTRY_NMOS_HEADERS
    nmos/activation_mode.h
    nmos/admin_ui.h
    nmos/api_downgrade.h
    nmos/api_utils.h
    nmos/api_version.h
    nmos/filesystem_route.h
    nmos/health.h
    nmos/id.h
    nmos/json_fields.h
    nmos/log_manip.h
    nmos/logging_api.h
    nmos/mdns.h
    nmos/mdns_api.h
    nmos/model.h
    nmos/node_api.h
    nmos/paging_utils.h
    nmos/process_utils.h
    nmos/query_api.h
    nmos/query_utils.h
    nmos/query_ws_api.h
    nmos/rational.h
    nmos/registration_api.h
    nmos/resource.h
    nmos/resources.h
    nmos/server_resources.h
    nmos/settings.h
    nmos/settings_api.h
    nmos/slog.h
    nmos/tai.h
    nmos/thread_utils.h
    nmos/type.h
    nmos/version.h
    )

set(NMOS_CPP_REGISTRY_RQL_SOURCES
    rql/rql.cpp
    )
set(NMOS_CPP_REGISTRY_RQL_HEADERS
    rql/rql.h
    )

set(NMOS_CPP_REGISTRY_SLOG_HEADERS
    slog/all_in_one.h
    )

add_executable(
    nmos-cpp-registry
    ${NMOS_CPP_REGISTRY_SOURCES}
    ${NMOS_CPP_REGISTRY_HEADERS}
    ${NMOS_CPP_REGISTRY_CPPREST_SOURCES}
    ${NMOS_CPP_REGISTRY_CPPREST_HEADERS}
    ${NMOS_CPP_REGISTRY_NMOS_SOURCES}
    ${NMOS_CPP_REGISTRY_NMOS_HEADERS}
    ${NMOS_CPP_REGISTRY_RQL_SOURCES}
    ${NMOS_CPP_REGISTRY_RQL_HEADERS}
    ${NMOS_CPP_REGISTRY_SLOG_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_REGISTRY_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_REGISTRY_NMOS_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_REGISTRY_RQL_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_REGISTRY_CPPREST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_REGISTRY_NMOS_HEADERS})
source_group("rql\\Header Files" FILES ${NMOS_CPP_REGISTRY_RQL_HEADERS})
source_group("slog\\Header Files" FILES ${NMOS_CPP_REGISTRY_SLOG_HEADERS})

target_link_libraries(
    nmos-cpp-registry
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

# nmos-cpp-registry-test

set(NMOS_CPP_REGISTRY_TEST_SOURCES
    nmos-cpp-registry/test/main.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_BST_TEST_SOURCES
    )
set(NMOS_CPP_REGISTRY_TEST_BST_TEST_HEADERS
    bst/test/test.h
    )

set(NMOS_CPP_REGISTRY_TEST_CPPREST_SOURCES
    cpprest/api_router.cpp
    cpprest/host_utils.cpp
    cpprest/http_utils.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_CPPREST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_SOURCES
    cpprest/test/api_router_test.cpp
    cpprest/test/http_utils_test.cpp
    cpprest/test/regex_utils_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_MDNS_TEST_SOURCES
    mdns/test/core_test.cpp
    mdns/test/mdns_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_MDNS_TEST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_NMOS_SOURCES
    nmos/api_utils.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_NMOS_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_NMOS_TEST_SOURCES
    nmos/test/api_utils_test.cpp
    nmos/test/paging_utils_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_NMOS_TEST_HEADERS
    )

add_executable(
    nmos-cpp-registry-test
    ${NMOS_CPP_REGISTRY_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_BST_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_BST_TEST_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_CPPREST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_CPPREST_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_NMOS_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_NMOS_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SOURCES})
source_group("bst\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_BST_TEST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_SOURCES})
source_group("cpprest\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_SOURCES})
source_group("mdns\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_SOURCES})
source_group("nmos\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_HEADERS})
source_group("bst\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_BST_TEST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_HEADERS})
source_group("cpprest\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_HEADERS})
source_group("mdns\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_HEADERS})
source_group("nmos\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_HEADERS})

target_link_libraries(
    nmos-cpp-registry-test
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

add_test(NAME nmos-cpp-registry-test COMMAND nmos-cpp-registry-test -r compact)
