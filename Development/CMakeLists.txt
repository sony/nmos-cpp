# CMake 3.9 is required due to cpprestsdk-config.cmake using find_dependency with COMPONENTS
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# project name
project(nmos-cpp)

# enable C++11
enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# note: to see the output of any failed tests, set CTEST_OUTPUT_ON_FAILURE=1 in the environment
# and also remember that CMake doesn't add dependencies to the "test" (or "RUN_TESTS") target
# so after changing code under test, it is important to "make all" (or build "ALL_BUILD")
enable_testing()

# location of additional cmake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    )

# guard against in-source builds and bad build-type strings
include(safeguards)

# find dependencies

# cpprestsdk
# note: 2.10.0 minimum but there's no cpprestsdk-configVersion.cmake
# and CPPREST_VERSION_MAJOR, etc. also aren't exported by cpprestsdk::cpprest
find_package(cpprestsdk REQUIRED NAMES cpprestsdk cpprest)
if(cpprestsdk_FOUND)
    message(STATUS "Found cpprestsdk")
endif()
get_target_property(CPPREST_INCLUDE_DIR cpprestsdk::cpprest INTERFACE_INCLUDE_DIRECTORIES)

# websocketpp
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

# boost
# note: some components are only required for one platform or other
set(FIND_BOOST_COMPONENTS system date_time regex)

# platform-specific dependencies

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # find bonjour (for mdns_static)
    # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR aren't set, the headers and library are assumed to be installed in the system paths
    set (BONJOUR_LIB libdns_sd.so)

    # add dependency required by nmos/filesystem_route.cpp
    if((CMAKE_CXX_COMPILER_ID MATCHES GNU) AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.3))
        set (PLATFORM_LIBS -lstdc++fs)
    else()
        list(APPEND FIND_BOOST_COMPONENTS filesystem)
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # find bonjour (for mdns_static)
    # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR are now set by default to the location used by the Bonjour SDK Installer (bonjoursdksetup.exe) 3.0.0
    set (BONJOUR_INCLUDE "$ENV{PROGRAMFILES}/Bonjour SDK/Include" CACHE PATH "Bonjour SDK include directory")
    set (BONJOUR_LIB_DIR "$ENV{PROGRAMFILES}/Bonjour SDK/Lib/x64" CACHE PATH "Bonjour SDK library directory")
    set (BONJOUR_LIB dnssd)

    # define _WIN32_WINNT because Boost.Asio gets terribly noisy otherwise
    # notes:
    #   cpprestsdk adds /D_WIN32_WINNT=0x0600 (Windows Vista) explicitly...
    #   calculating the value from CMAKE_SYSTEM_VERSION might be better?
    #   adding a force include for <ssdkddkver.h> could be another option
    # see:
    #   https://docs.microsoft.com/en-gb/cpp/porting/modifying-winver-and-win32-winnt
    #   https://stackoverflow.com/questions/9742003/platform-detection-in-cmake
    add_definitions(/D_WIN32_WINNT=0x0600)

    # does one of our dependencies result in needing to do this?
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt")
endif()

# since std::shared_mutex is not available until C++17
list(APPEND FIND_BOOST_COMPONENTS thread)
add_definitions(/DBST_SHARED_MUTEX_BOOST)

# find boost
# note: 1.57.0 doesn't work due to https://svn.boost.org/trac10/ticket/10754
find_package(Boost 1.54.0 REQUIRED COMPONENTS ${FIND_BOOST_COMPONENTS})

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
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")	
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJSON_SCHEMA_BOOST_REGEX")	
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -D_GLIBCXX_USE_NANOSLEEP -DBOOST_RESULT_OF_USE_DECLTYPE -DSLOG_DETAIL_NO_REF_QUALIFIERS -DJSON_SCHEMA_BOOST_REGEX")	
    endif()
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
    ${PROJECT_SOURCE_DIR}/third_party
    ${CPPREST_INCLUDE_DIR} # defined above from target cpprestsdk::cpprest of find_package(cpprestsdk)
    ${WEBSOCKETPP_INCLUDE_DIR} # defined by find_package(websocketpp)
    ${Boost_INCLUDE_DIRS} # defined by find_package(Boost) 
    ${BONJOUR_INCLUDE} # defined above
    ${PROJECT_SOURCE_DIR}/third_party/nlohmann
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

# ensure e.g. target_compile_definitions for cppprestsdk::cpprest are applied when building this target
target_link_libraries(
    mdns_static
    cpprestsdk::cpprest
    )

# nmos_is04_schemas library

set(NMOS_IS04_SCHEMAS_HEADERS
    nmos/is04_schemas/is04_schemas.h
    )

set(NMOS_IS04_V1_2_SCHEMAS_JSON
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/clock_internal.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/clock_ptp.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/device.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/devices.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/empty.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/error.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flows.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_audio.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_audio_coded.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_audio_raw.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_core.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_data.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_mux.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_sdianc_data.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_video.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_video_coded.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/flow_video_raw.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/node.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/nodeapi-base.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/nodes.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/queryapi-base.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/queryapi-subscription-response.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/queryapi-subscriptions-post-request.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/queryapi-subscriptions-response.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/queryapi-subscriptions-websocket.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receiver.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receivers.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receiver_audio.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receiver_core.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receiver_data.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receiver_mux.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/receiver_video.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/registrationapi-base.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/registrationapi-health-response.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/registrationapi-resource-post-request.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/registrationapi-resource-response.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/resource_core.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/sender.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/senders.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/source.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/sources.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/source_audio.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/source_core.json
    third_party/nmos-discovery-registration/v1.2/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_1_1_SCHEMAS_JSON
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/clock_internal.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/clock_ptp.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/device.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/devices.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/empty.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/error.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flows.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_audio.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_audio_coded.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_audio_raw.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_core.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_data.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_mux.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_sdianc_data.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_video.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_video_coded.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/flow_video_raw.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/node.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/nodeapi-base.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/nodes.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/queryapi-base.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/queryapi-subscription-response.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/queryapi-subscriptions-post-request.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/queryapi-subscriptions-response.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/queryapi-subscriptions-websocket.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receiver.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receivers.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receiver_audio.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receiver_core.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receiver_data.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receiver_mux.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/receiver_video.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/registrationapi-base.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/registrationapi-health-response.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/registrationapi-resource-post-request.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/registrationapi-resource-response.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/resource_core.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/sender.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/senders.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/source.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/sources.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/source_audio.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/source_core.json
    third_party/nmos-discovery-registration/v1.1.1/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_0_2_SCHEMAS_JSON
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/device.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/devices.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/empty.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/error.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/flow.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/flows.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/node.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/nodeapi-base.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/nodes.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/queryapi-base.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/queryapi-subscription-response.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/queryapi-subscriptions-response.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/queryapi-v1.0-subscriptions-post-request.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/queryapi-v1.0-subscriptions-websocket.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/receiver.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/receivers.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/registrationapi-base.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/registrationapi-health-response.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/registrationapi-resource-response.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/registrationapi-v1.0-resource-post-request.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/sender-target.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/sender.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/senders.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/source.json
    third_party/nmos-discovery-registration/v1.0.2/APIs/schemas/sources.json
    )

set(NMOS_IS04_SCHEMAS_JSON_MATCH "third_party/nmos-discovery-registration/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS04_SCHEMAS_SOURCE_REPLACE "${CMAKE_BINARY_DIR}/nmos/is04_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_2_SCHEMAS_SOURCES "${NMOS_IS04_V1_2_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_1_1_SCHEMAS_SOURCES "${NMOS_IS04_V1_1_1_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_0_2_SCHEMAS_SOURCES "${NMOS_IS04_V1_0_2_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS04_V1_2_SCHEMAS_JSON} ${NMOS_IS04_V1_1_1_SCHEMAS_JSON} ${NMOS_IS04_V1_0_2_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is04_schemas\n\
    {\n\
        namespace ${NS}\n\
        {\n\
            const char* ${VAR} = R\"-auto-generated-(")

    file(READ "${JSON}" RAW)
    file(APPEND "${SOURCE}.in" "${RAW}")

    file(APPEND "${SOURCE}.in" ")-auto-generated-\";\n\
        }\n\
    }\n\
}\n")

    configure_file("${SOURCE}.in" "${SOURCE}" COPYONLY)
endforeach()

add_library(
    nmos_is04_schemas_static STATIC
    ${NMOS_IS04_SCHEMAS_HEADERS}
    ${NMOS_IS04_V1_2_SCHEMAS_SOURCES}
    ${NMOS_IS04_V1_1_1_SCHEMAS_SOURCES}
    ${NMOS_IS04_V1_0_2_SCHEMAS_SOURCES}
    )

source_group("nmos\\is04_schemas\\Header Files" FILES ${NMOS_IS04_SCHEMAS_HEADERS})
source_group("nmos\\is04_schemas\\v1.2\\Source Files" FILES ${NMOS_IS04_V1_2_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\v1.1.1\\Source Files" FILES ${NMOS_IS04_V1_1_1_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\v1.0.2\\Source Files" FILES ${NMOS_IS04_V1_0_2_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is04_schemas_static
    )

# json schema validator library

set(JSON_SCHEMA_VALIDATOR_SOURCES
    third_party/nlohmann/json-validator.cpp
    third_party/nlohmann/json-uri.cpp
    )

set(JSON_SCHEMA_VALIDATOR_HEADERS
    third_party/nlohmann/json-schema.hpp
    third_party/nlohmann/json.hpp
    )

add_library(
    json_schema_validator_static STATIC
    ${JSON_SCHEMA_VALIDATOR_SOURCES}
    ${JSON_SCHEMA_VALIDATOR_HEADERS}
	)

source_group("Source Files" FILES ${JSON_SCHEMA_VALIDATOR_SOURCES})
source_group("Header Files" FILES ${JSON_SCHEMA_VALIDATOR_HEADERS})

target_link_libraries(
    json_schema_validator_static
    )

# nmos-cpp-node

set(NMOS_CPP_NODE_SOURCES
    nmos-cpp-node/main.cpp
    )
set(NMOS_CPP_NODE_HEADERS
    nmos-cpp-node/main_gate.h
    )

set(NMOS_CPP_NODE_BST_SOURCES
    )
set(NMOS_CPP_NODE_BST_HEADERS
    bst/regex.h
    bst/shared_mutex.h
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
    nmos/node_behaviour.cpp
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
    nmos/copyable_atomic.h
    nmos/connection_api.h
    nmos/health.h
    nmos/id.h
    nmos/json_fields.h
    nmos/log_manip.h
    nmos/logging_api.h
    nmos/mdns.h
    nmos/model.h
    nmos/mutex.h
    nmos/node_api.h
    nmos/node_behaviour.h
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

set(NMOS_CPP_NODE_PPLX_SOURCES
    pplx/pplx_utils.cpp
    )
set(NMOS_CPP_NODE_PPLX_HEADERS
    pplx/pplx_utils.h
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
    ${NMOS_CPP_NODE_BST_SOURCES}
    ${NMOS_CPP_NODE_BST_HEADERS}
    ${NMOS_CPP_NODE_CPPREST_SOURCES}
    ${NMOS_CPP_NODE_CPPREST_HEADERS}
    ${NMOS_CPP_NODE_NMOS_SOURCES}
    ${NMOS_CPP_NODE_NMOS_HEADERS}
    ${NMOS_CPP_NODE_PPLX_SOURCES}
    ${NMOS_CPP_NODE_PPLX_HEADERS}
    ${NMOS_CPP_NODE_RQL_SOURCES}
    ${NMOS_CPP_NODE_RQL_HEADERS}
    ${NMOS_CPP_NODE_SLOG_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_NODE_SOURCES})
source_group("bst\\Source Files" FILES ${NMOS_CPP_NODE_BST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_NODE_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_NODE_NMOS_SOURCES})
source_group("pplx\\Source Files" FILES ${NMOS_CPP_NODE_PPLX_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_NODE_RQL_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_NODE_HEADERS})
source_group("bst\\Header Files" FILES ${NMOS_CPP_NODE_BST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_NODE_CPPREST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_NODE_NMOS_HEADERS})
source_group("pplx\\Header Files" FILES ${NMOS_CPP_NODE_PPLX_HEADERS})
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

set(NMOS_CPP_REGISTRY_BST_SOURCES
    )
set(NMOS_CPP_REGISTRY_BST_HEADERS
    bst/filesystem.h
    bst/regex.h
    bst/shared_mutex.h
    )

set(NMOS_CPP_REGISTRY_CPPREST_SOURCES
    cpprest/api_router.cpp
    cpprest/host_utils.cpp
    cpprest/http_utils.cpp
    cpprest/json_utils.cpp
	cpprest/json_validator_impl.cpp
    cpprest/ws_listener_impl.cpp
    )
set(NMOS_CPP_REGISTRY_CPPREST_HEADERS
    cpprest/api_router.h
    cpprest/basic_utils.h
    cpprest/host_utils.h
    cpprest/http_utils.h
    cpprest/json_utils.h
	cpprest/json_validator.h
    cpprest/logging_utils.h
    cpprest/regex_utils.h
    cpprest/ws_listener.h
    )

set(NMOS_CPP_REGISTRY_NMOS_SOURCES
    nmos/admin_ui.cpp
    nmos/api_downgrade.cpp
    nmos/api_utils.cpp
    nmos/filesystem_route.cpp
    nmos/json_schema.cpp
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
    nmos/copyable_atomic.h
    nmos/filesystem_route.h
    nmos/health.h
    nmos/id.h
    nmos/json_fields.h
    nmos/json_schema.h
    nmos/log_manip.h
    nmos/logging_api.h
    nmos/mdns.h
    nmos/mdns_api.h
    nmos/model.h
    nmos/mutex.h
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
    ${NMOS_CPP_REGISTRY_BST_SOURCES}
    ${NMOS_CPP_REGISTRY_BST_HEADERS}
    ${NMOS_CPP_REGISTRY_CPPREST_SOURCES}
    ${NMOS_CPP_REGISTRY_CPPREST_HEADERS}
    ${NMOS_CPP_REGISTRY_NMOS_SOURCES}
    ${NMOS_CPP_REGISTRY_NMOS_HEADERS}
    ${NMOS_CPP_REGISTRY_RQL_SOURCES}
    ${NMOS_CPP_REGISTRY_RQL_HEADERS}
    ${NMOS_CPP_REGISTRY_SLOG_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_SOURCES})
source_group("bst\\Source Files" FILES ${NMOS_CPP_REGISTRY_BST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_REGISTRY_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_REGISTRY_NMOS_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_REGISTRY_RQL_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_HEADERS})
source_group("bst\\Header Files" FILES ${NMOS_CPP_REGISTRY_BST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_REGISTRY_CPPREST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_REGISTRY_NMOS_HEADERS})
source_group("rql\\Header Files" FILES ${NMOS_CPP_REGISTRY_RQL_HEADERS})
source_group("slog\\Header Files" FILES ${NMOS_CPP_REGISTRY_SLOG_HEADERS})

target_link_libraries(
    nmos-cpp-registry
    mdns_static
    cpprestsdk::cpprest
	json_schema_validator_static
	nmos_is04_schemas_static
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

include(Catch)

catch_discover_tests(nmos-cpp-registry-test EXTRA_ARGS -r compact)
