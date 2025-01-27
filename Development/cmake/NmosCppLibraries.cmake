include(CMakeRegexEscape)

string(REGEX REPLACE ${REPLACE_MATCH} ${REPLACE_REPLACE} CMAKE_CURRENT_BINARY_DIR_REPLACE "${CMAKE_CURRENT_BINARY_DIR}")

# detail headers

set(DETAIL_HEADERS
    detail/default_init_allocator.h
    detail/for_each_reversed.h
    detail/pragma_warnings.h
    detail/private_access.h
    )

install(FILES ${DETAIL_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/detail)

# slog library

# compile-time control of logging loquacity
# use slog::never_log_severity to strip all logging at compile-time, or slog::max_verbosity for full control at run-time
set(SLOG_LOGGING_SEVERITY slog::max_verbosity CACHE STRING "Compile-time logging level, e.g. between 40 (least verbose, only fatal messages) and -40 (most verbose)")
mark_as_advanced(FORCE SLOG_LOGGING_SEVERITY)

set(SLOG_HEADERS
    slog/all_in_one.h
    )

add_library(slog INTERFACE)

target_compile_definitions(
    slog INTERFACE
    SLOG_STATIC
    SLOG_LOGGING_SEVERITY=${SLOG_LOGGING_SEVERITY}
    )
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        target_compile_definitions(
            slog INTERFACE
            SLOG_DETAIL_NO_REF_QUALIFIERS
            )
    endif()
endif()
target_include_directories(slog INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

install(FILES ${SLOG_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/slog)

list(APPEND NMOS_CPP_TARGETS slog)
add_library(nmos-cpp::slog ALIAS slog)

# mDNS support library

set(MDNS_SOURCES
    mdns/core.cpp
    mdns/dns_sd_impl.cpp
    mdns/service_advertiser_impl.cpp
    mdns/service_discovery_impl.cpp
    )
set(MDNS_HEADERS
    mdns/core.h
    mdns/dns_sd_impl.h
    mdns/service_advertiser.h
    mdns/service_advertiser_impl.h
    mdns/service_discovery.h
    mdns/service_discovery_impl.h
    )

add_library(
    mdns STATIC
    ${MDNS_SOURCES}
    ${MDNS_HEADERS}
    )

source_group("mdns\\Source Files" FILES ${MDNS_SOURCES})
source_group("mdns\\Header Files" FILES ${MDNS_HEADERS})

target_link_libraries(
    mdns PRIVATE
    nmos-cpp::compile-settings
    )
target_link_libraries(
    mdns PUBLIC
    nmos-cpp::slog
    nmos-cpp::cpprestsdk
    nmos-cpp::Boost
    )
# CMake 3.17 is required in order to get the INTERFACE_LINK_OPTIONS
# see https://cmake.org/cmake/help/latest/policy/CMP0099.html
target_link_libraries(
    mdns PRIVATE
    nmos-cpp::DNSSD
    )
target_include_directories(mdns PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

install(FILES ${MDNS_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/mdns)

list(APPEND NMOS_CPP_TARGETS mdns)
add_library(nmos-cpp::mdns ALIAS mdns)

# LLDP support library
if(NMOS_CPP_BUILD_LLDP)
    set(LLDP_SOURCES
        lldp/lldp.cpp
        lldp/lldp_frame.cpp
        lldp/lldp_manager_impl.cpp
        )
    set(LLDP_HEADERS
        lldp/lldp.h
        lldp/lldp_frame.h
        lldp/lldp_manager.h
        )

    add_library(
        lldp STATIC
        ${LLDP_SOURCES}
        ${LLDP_HEADERS}
        )

    source_group("lldp\\Source Files" FILES ${LLDP_SOURCES})
    source_group("lldp\\Header Files" FILES ${LLDP_HEADERS})

    target_link_libraries(
        lldp PRIVATE
        nmos-cpp::compile-settings
        )
    target_link_libraries(
        lldp PUBLIC
        nmos-cpp::slog
        nmos-cpp::cpprestsdk
        )
    target_link_libraries(
        lldp PRIVATE
        nmos-cpp::PCAP
        nmos-cpp::Boost
        )
    target_include_directories(lldp PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
        )
    target_compile_definitions(
        lldp INTERFACE
        HAVE_LLDP
        )

    install(FILES ${LLDP_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/lldp)

    list(APPEND NMOS_CPP_TARGETS lldp)
    add_library(nmos-cpp::lldp ALIAS lldp)
endif()

# nmos_is04_schemas library

set(NMOS_IS04_SCHEMAS_HEADERS
    nmos/is04_schemas/is04_schemas.h
    )

set(NMOS_IS04_V1_3_TAG v1.3.x)
set(NMOS_IS04_V1_2_TAG v1.2.x)
set(NMOS_IS04_V1_1_TAG v1.1.x)
set(NMOS_IS04_V1_0_TAG v1.0.x)

set(NMOS_IS04_V1_3_SCHEMAS_JSON
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/clock_internal.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/clock_ptp.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/device.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/devices.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/error.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flows.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_audio.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_audio_coded.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_audio_raw.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_core.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_data.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_json_data.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_mux.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_sdianc_data.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_video.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_video_coded.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_video_raw.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/node.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/nodeapi-base.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/nodeapi-receiver-target.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/nodes.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-base.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscription-response.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receivers.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_audio.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_core.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_data.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_mux.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_video.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-base.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-health-response.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-resource-response.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/resource_core.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/sender.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/senders.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/sources.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_audio.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_core.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_data.json
    third_party/is-04/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_2_SCHEMAS_JSON
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/clock_internal.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/clock_ptp.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/device.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/devices.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/error.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flows.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio_coded.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio_raw.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_core.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_data.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_mux.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_sdianc_data.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video_coded.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video_raw.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/node.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodeapi-base.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodeapi-receiver-target.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodes.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-base.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscription-response.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receivers.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_audio.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_core.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_data.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_mux.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_video.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-base.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-health-response.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-resource-response.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/resource_core.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/sender.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/senders.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/sources.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_audio.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_core.json
    third_party/is-04/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_1_SCHEMAS_JSON
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/clock_internal.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/clock_ptp.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/device.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/devices.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/error.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flows.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio_coded.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio_raw.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_core.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_data.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_mux.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_sdianc_data.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video_coded.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video_raw.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/node.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodeapi-base.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodeapi-receiver-target.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodes.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-base.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscription-response.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receivers.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_audio.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_core.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_data.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_mux.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_video.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-base.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-health-response.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-resource-response.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/resource_core.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/sender.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/senders.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/sources.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_audio.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_core.json
    third_party/is-04/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_0_SCHEMAS_JSON
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/device.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/devices.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/error.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/flow.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/flows.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/node.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodeapi-base.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodeapi-receiver-target.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodes.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-base.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-subscription-response.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-v1.0-subscriptions-post-request.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-v1.0-subscriptions-websocket.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/receiver.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/receivers.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-base.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-health-response.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-resource-response.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-v1.0-resource-post-request.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sender-target.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sender.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/senders.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/source.json
    third_party/is-04/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sources.json
    )

set(NMOS_IS04_SCHEMAS_JSON_MATCH "third_party/is-04/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS04_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is04_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_3_SCHEMAS_SOURCES "${NMOS_IS04_V1_3_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_2_SCHEMAS_SOURCES "${NMOS_IS04_V1_2_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_1_SCHEMAS_SOURCES "${NMOS_IS04_V1_1_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_0_SCHEMAS_SOURCES "${NMOS_IS04_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS04_V1_3_SCHEMAS_JSON} ${NMOS_IS04_V1_2_SCHEMAS_JSON} ${NMOS_IS04_V1_1_SCHEMAS_JSON} ${NMOS_IS04_V1_0_SCHEMAS_JSON})
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
    nmos_is04_schemas STATIC
    ${NMOS_IS04_SCHEMAS_HEADERS}
    ${NMOS_IS04_V1_3_SCHEMAS_SOURCES}
    ${NMOS_IS04_V1_2_SCHEMAS_SOURCES}
    ${NMOS_IS04_V1_1_SCHEMAS_SOURCES}
    ${NMOS_IS04_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is04_schemas\\Header Files" FILES ${NMOS_IS04_SCHEMAS_HEADERS})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_3_TAG}\\Source Files" FILES ${NMOS_IS04_V1_3_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_2_TAG}\\Source Files" FILES ${NMOS_IS04_V1_2_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_1_TAG}\\Source Files" FILES ${NMOS_IS04_V1_1_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_0_TAG}\\Source Files" FILES ${NMOS_IS04_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is04_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is04_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is04_schemas)
add_library(nmos-cpp::nmos_is04_schemas ALIAS nmos_is04_schemas)

# nmos_is05_schemas library

set(NMOS_IS05_SCHEMAS_HEADERS
    nmos/is05_schemas/is05_schemas.h
    )

set(NMOS_IS05_V1_1_TAG v1.1.x)
set(NMOS_IS05_V1_0_TAG v1.0.x)

set(NMOS_IS05_V1_1_SCHEMAS_JSON
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/activation-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/activation-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/bulk-receiver-post-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/bulk-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/bulk-sender-post-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-base.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-bulk.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-receiver.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-sender.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-single.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraint-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema-mqtt.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema-rtp.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema-websocket.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/error.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_dash.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_ext.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_mqtt.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_rtp.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_websocket.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver-stage-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver-transport-file.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_dash.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_ext.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_mqtt.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_rtp.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_websocket.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender-receiver-base.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender-stage-schema.json
    third_party/is-05/${NMOS_IS05_V1_1_TAG}/APIs/schemas/transporttype-response-schema.json
    )

set(NMOS_IS05_V1_0_SCHEMAS_JSON
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-base.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-bulk.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-receiver.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-sender.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-single.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/error.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_receiver_transport_params_dash.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_receiver_transport_params_rtp.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_sender_transport_params_dash.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_sender_transport_params_rtp.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-activation-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-activation-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-bulk-receiver-post-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-bulk-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-bulk-sender-post-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-constraints-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-receiver-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-receiver-stage-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/sender-receiver-base.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-sender-response-schema.json
    third_party/is-05/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-sender-stage-schema.json
    )

set(NMOS_IS05_SCHEMAS_JSON_MATCH "third_party/is-05/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS05_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is05_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS05_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS05_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS05_V1_1_SCHEMAS_SOURCES "${NMOS_IS05_V1_1_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS05_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS05_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS05_V1_0_SCHEMAS_SOURCES "${NMOS_IS05_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS05_V1_1_SCHEMAS_JSON} ${NMOS_IS05_V1_0_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS05_SCHEMAS_JSON_MATCH}" "${NMOS_IS05_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS05_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS05_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is05_schemas\n\
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
    nmos_is05_schemas STATIC
    ${NMOS_IS05_SCHEMAS_HEADERS}
    ${NMOS_IS05_V1_1_SCHEMAS_SOURCES}
    ${NMOS_IS05_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is05_schemas\\Header Files" FILES ${NMOS_IS05_SCHEMAS_HEADERS})
source_group("nmos\\is05_schemas\\${NMOS_IS05_V1_1_TAG}\\Source Files" FILES ${NMOS_IS05_V1_1_SCHEMAS_SOURCES})
source_group("nmos\\is05_schemas\\${NMOS_IS05_V1_0_TAG}\\Source Files" FILES ${NMOS_IS05_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is05_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is05_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is05_schemas)
add_library(nmos-cpp::nmos_is05_schemas ALIAS nmos_is05_schemas)

# nmos_is08_schemas library

set(NMOS_IS08_SCHEMAS_HEADERS
    nmos/is08_schemas/is08_schemas.h
    )

set(NMOS_IS08_V1_0_TAG v1.0.x)

set(NMOS_IS08_V1_0_SCHEMAS_JSON
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/activation-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/activation-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/base-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/error.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/input-base-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/input-caps-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/input-channels-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/input-parent-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/input-properties-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/inputs-outputs-base-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/io-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-activations-activation-get-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-activations-get-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-activations-post-request-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-activations-post-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-active-output-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-active-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-base-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/map-entries-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/output-base-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/output-caps-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/output-channels-response-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/output-properties-schema.json
    third_party/is-08/${NMOS_IS08_V1_0_TAG}/APIs/schemas/output-sourceid-response-schema.json
    )

set(NMOS_IS08_SCHEMAS_JSON_MATCH "third_party/is-08/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS08_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is08_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS08_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS08_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS08_V1_0_SCHEMAS_SOURCES "${NMOS_IS08_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS08_V1_0_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS08_SCHEMAS_JSON_MATCH}" "${NMOS_IS08_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS08_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS08_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is08_schemas\n\
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
    nmos_is08_schemas STATIC
    ${NMOS_IS08_SCHEMAS_HEADERS}
    ${NMOS_IS08_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is08_schemas\\Header Files" FILES ${NMOS_IS08_SCHEMAS_HEADERS})
source_group("nmos\\is08_schemas\\${NMOS_IS08_V1_0_TAG}\\Source Files" FILES ${NMOS_IS08_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is08_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is08_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is08_schemas)
add_library(nmos-cpp::nmos_is08_schemas ALIAS nmos_is08_schemas)

# nmos_is09_schemas library

set(NMOS_IS09_SCHEMAS_HEADERS
    nmos/is09_schemas/is09_schemas.h
    )

set(NMOS_IS09_V1_0_TAG v1.0.x)

set(NMOS_IS09_V1_0_SCHEMAS_JSON
    third_party/is-09/${NMOS_IS09_V1_0_TAG}/APIs/schemas/base.json
    third_party/is-09/${NMOS_IS09_V1_0_TAG}/APIs/schemas/error.json
    third_party/is-09/${NMOS_IS09_V1_0_TAG}/APIs/schemas/global.json
    third_party/is-09/${NMOS_IS09_V1_0_TAG}/APIs/schemas/resource_core.json
    )

set(NMOS_IS09_SCHEMAS_JSON_MATCH "third_party/is-09/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS09_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is09_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS09_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS09_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS09_V1_0_SCHEMAS_SOURCES "${NMOS_IS09_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS09_V1_0_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS09_SCHEMAS_JSON_MATCH}" "${NMOS_IS09_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS09_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS09_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is09_schemas\n\
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
    nmos_is09_schemas STATIC
    ${NMOS_IS09_SCHEMAS_HEADERS}
    ${NMOS_IS09_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is09_schemas\\Header Files" FILES ${NMOS_IS09_SCHEMAS_HEADERS})
source_group("nmos\\is09_schemas\\${NMOS_IS09_V1_0_TAG}\\Source Files" FILES ${NMOS_IS09_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is09_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is09_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is09_schemas)
add_library(nmos-cpp::nmos_is09_schemas ALIAS nmos_is09_schemas)

# nmos_is10_schemas library

set(NMOS_IS10_SCHEMAS_HEADERS
    nmos/is10_schemas/is10_schemas.h
    )

set(NMOS_IS10_V1_0_TAG v1.0.x)

set(NMOS_IS10_V1_0_SCHEMAS_JSON
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/auth_metadata.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/jwks_response.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/jwks_schema.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/register_client_error_response.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/register_client_request.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/register_client_response.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/token_error_response.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/token_response.json
    third_party/is-10/${NMOS_IS10_V1_0_TAG}/APIs/schemas/token_schema.json
    )

set(NMOS_IS10_SCHEMAS_JSON_MATCH "third_party/is-10/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS10_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is10_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS10_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS10_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS10_V1_0_SCHEMAS_SOURCES "${NMOS_IS10_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS10_V1_0_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS10_SCHEMAS_JSON_MATCH}" "${NMOS_IS10_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS10_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS10_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is10_schemas\n\
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
    nmos_is10_schemas STATIC
    ${NMOS_IS10_SCHEMAS_HEADERS}
    ${NMOS_IS10_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is10_schemas\\Header Files" FILES ${NMOS_IS10_SCHEMAS_HEADERS})
source_group("nmos\\is10_schemas\\${NMOS_IS10_V1_0_TAG}\\Source Files" FILES ${NMOS_IS10_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is10_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is10_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is10_schemas)
add_library(nmos-cpp::nmos_is10_schemas ALIAS nmos_is10_schemas)

# nmos_is12_schemas library

set(NMOS_IS12_SCHEMAS_HEADERS
    nmos/is12_schemas/is12_schemas.h
    )

set(NMOS_IS12_V1_0_TAG v1.0.x)

set(NMOS_IS12_V1_0_SCHEMAS_JSON
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/base-message.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/command-message.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/command-response-message.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/error-message.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/event-data.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/notification-message.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/property-changed-event-data.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/subscription-message.json
    third_party/is-12/${NMOS_IS12_V1_0_TAG}/APIs/schemas/subscription-response-message.json
    )

set(NMOS_IS12_SCHEMAS_JSON_MATCH "third_party/is-12/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS12_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is12_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS12_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS12_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS12_V1_0_SCHEMAS_SOURCES "${NMOS_IS12_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS12_V1_0_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS12_SCHEMAS_JSON_MATCH}" "${NMOS_IS12_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS12_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS12_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is12_schemas\n\
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
    nmos_is12_schemas STATIC
    ${NMOS_IS12_SCHEMAS_HEADERS}
    ${NMOS_IS12_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is12_schemas\\Header Files" FILES ${NMOS_IS12_SCHEMAS_HEADERS})
source_group("nmos\\is12_schemas\\${NMOS_IS12_V1_0_TAG}\\Source Files" FILES ${NMOS_IS12_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is12_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is12_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is12_schemas)
add_library(nmos-cpp::nmos_is12_schemas ALIAS nmos_is12_schemas)

# nmos_is14_schemas library

set(NMOS_IS14_SCHEMAS_HEADERS
    nmos/is14_schemas/is14_schemas.h
    )

set(NMOS_IS14_V1_0_TAG v1.0.x)

set(NMOS_IS14_V1_0_SCHEMAS_JSON
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/base.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/bulkProperties-get-response.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/bulkProperties-set-request.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/bulkProperties-set-response.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/bulkProperties-validate-request.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/bulkProperties-validate-response.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/descriptor-get.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/method-patch-request.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/method-patch-response.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/methods-base.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/ms05-error.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/properties-base.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/property.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/property-descriptor.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/property-value-get.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/property-value-put-request.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/property-value-put-response.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/rolePath.json
    third_party/is-14/${NMOS_IS14_V1_0_TAG}/APIs/schemas/rolePaths-base.json
    )

set(NMOS_IS14_SCHEMAS_JSON_MATCH "third_party/is-14/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS14_SCHEMAS_SOURCE_REPLACE "${CMAKE_CURRENT_BINARY_DIR_REPLACE}/nmos/is14_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS14_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS14_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS14_V1_0_SCHEMAS_SOURCES "${NMOS_IS14_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS14_V1_0_SCHEMAS_JSON})
    string(REGEX REPLACE "${NMOS_IS14_SCHEMAS_JSON_MATCH}" "${NMOS_IS14_SCHEMAS_SOURCE_REPLACE}" SOURCE "${JSON}")
    string(REGEX REPLACE "${NMOS_IS14_SCHEMAS_JSON_MATCH}" "\\1" NS "${JSON}")
    string(REGEX REPLACE "${NMOS_IS14_SCHEMAS_JSON_MATCH}" "\\2" VAR "${JSON}")
    string(MAKE_C_IDENTIFIER "${NS}" NS)
    string(MAKE_C_IDENTIFIER "${VAR}" VAR)

    file(WRITE "${SOURCE}.in" "\
// Auto-generated from: ${JSON}\n\
\n\
namespace nmos\n\
{\n\
    namespace is14_schemas\n\
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
    nmos_is14_schemas STATIC
    ${NMOS_IS14_SCHEMAS_HEADERS}
    ${NMOS_IS14_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is14_schemas\\Header Files" FILES ${NMOS_IS14_SCHEMAS_HEADERS})
source_group("nmos\\is14_schemas\\${NMOS_IS14_V1_0_TAG}\\Source Files" FILES ${NMOS_IS14_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is14_schemas PRIVATE
    nmos-cpp::compile-settings
    )
target_include_directories(nmos_is14_schemas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

list(APPEND NMOS_CPP_TARGETS nmos_is14_schemas)
add_library(nmos-cpp::nmos_is14_schemas ALIAS nmos_is14_schemas)

# nmos-cpp library

set(NMOS_CPP_BST_SOURCES
    )
set(NMOS_CPP_BST_HEADERS
    bst/any.h
    bst/filesystem.h
    bst/optional.h
    bst/regex.h
    bst/shared_mutex.h
    )

set(NMOS_CPP_CPPREST_SOURCES
    cpprest/api_router.cpp
    cpprest/host_utils.cpp
    cpprest/http_utils.cpp
    cpprest/json_escape.cpp
    cpprest/json_storage.cpp
    cpprest/json_utils.cpp
    cpprest/json_validator_impl.cpp
    cpprest/json_visit.cpp
    cpprest/ws_listener_impl.cpp
    )

if(MSVC)
    # workaround for "fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj"
    set_source_files_properties(cpprest/ws_listener_impl.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

set(NMOS_CPP_CPPREST_HEADERS
    cpprest/access_token_error.h
    cpprest/api_router.h
    cpprest/basic_utils.h
    cpprest/client_type.h
    cpprest/code_challenge_method.h
    cpprest/grant_type.h
    cpprest/host_utils.h
    cpprest/http_utils.h
    cpprest/json_escape.h
    cpprest/json_ops.h
    cpprest/json_storage.h
    cpprest/json_utils.h
    cpprest/json_validator.h
    cpprest/json_visit.h
    cpprest/logging_utils.h
    cpprest/regex_utils.h
    cpprest/resource_server_error.h
    cpprest/response_type.h
    cpprest/token_endpoint_auth_method.h
    cpprest/uri_schemes.h
    cpprest/ws_listener.h
    cpprest/ws_utils.h
    )

set(NMOS_CPP_CPPREST_DETAILS_HEADERS
    cpprest/details/boost_u_workaround.h
    cpprest/details/pop_u.h
    cpprest/details/push_undef_u.h
    cpprest/details/system_error.h
    )

set(NMOS_CPP_JWK_HEADERS
    jwk/algorithm.h
    jwk/public_key_use.h
    )

set(NMOS_CPP_NMOS_SOURCES
    nmos/activation_utils.cpp
    nmos/admin_ui.cpp
    nmos/api_downgrade.cpp
    nmos/api_utils.cpp
    nmos/authorization.cpp
    nmos/authorization_handlers.cpp
    nmos/authorization_redirect_api.cpp
    nmos/authorization_behaviour.cpp
    nmos/authorization_operation.cpp
    nmos/authorization_state.cpp
    nmos/authorization_utils.cpp
    nmos/authorization_behaviour.cpp
    nmos/capabilities.cpp
    nmos/certificate_handlers.cpp
    nmos/channelmapping_activation.cpp
    nmos/channelmapping_api.cpp
    nmos/channelmapping_resources.cpp
    nmos/channels.cpp
    nmos/client_utils.cpp
    nmos/components.cpp
    nmos/configuration_api.cpp
    nmos/configuration_methods.cpp
	nmos/configuration_resources.cpp
    nmos/configuration_utils.cpp
    nmos/connection_activation.cpp
    nmos/connection_api.cpp
    nmos/connection_events_activation.cpp
    nmos/connection_resources.cpp
    nmos/control_protocol_handlers.cpp
    nmos/control_protocol_methods.cpp
    nmos/control_protocol_resource.cpp
    nmos/control_protocol_resources.cpp
    nmos/control_protocol_state.cpp
    nmos/control_protocol_utils.cpp
    nmos/control_protocol_ws_api.cpp
    nmos/did_sdid.cpp
    nmos/events_api.cpp
    nmos/events_resources.cpp
    nmos/events_ws_api.cpp
    nmos/events_ws_client.cpp
    nmos/filesystem_route.cpp
    nmos/group_hint.cpp
    nmos/id.cpp
    nmos/lldp_handler.cpp
    nmos/lldp_manager.cpp
    nmos/json_schema.cpp
    nmos/jwt_generator_impl.cpp
    nmos/jwk_utils.cpp
    nmos/jwks_uri_api.cpp
    nmos/jwt_validator_impl.cpp
    nmos/log_model.cpp
    nmos/logging_api.cpp
    nmos/manifest_api.cpp
    nmos/mdns.cpp
    nmos/mdns_api.cpp
    nmos/node_api.cpp
    nmos/node_api_target_handler.cpp
    nmos/node_behaviour.cpp
    nmos/node_interfaces.cpp
    nmos/node_resource.cpp
    nmos/node_resources.cpp
    nmos/node_server.cpp
    nmos/node_system_behaviour.cpp
    nmos/ocsp_behaviour.cpp
    nmos/ocsp_response_handler.cpp
    nmos/ocsp_utils.cpp
    nmos/process_utils.cpp
    nmos/query_api.cpp
    nmos/query_utils.cpp
    nmos/query_ws_api.cpp
    nmos/rational.cpp
    nmos/registration_api.cpp
    nmos/registry_resources.cpp
    nmos/registry_server.cpp
    nmos/resource.cpp
    nmos/resources.cpp
    nmos/schemas_api.cpp
    nmos/sdp_attributes.cpp
    nmos/sdp_utils.cpp
    nmos/server.cpp
    nmos/server_utils.cpp
    nmos/settings.cpp
    nmos/settings_api.cpp
    nmos/system_api.cpp
    nmos/system_resources.cpp
    nmos/video_jxsv.cpp
    nmos/ws_api_utils.cpp
    )
set(NMOS_CPP_NMOS_HEADERS
    nmos/activation_mode.h
    nmos/activation_utils.h
    nmos/admin_ui.h
    nmos/api_downgrade.h
    nmos/api_utils.h
    nmos/api_version.h
    nmos/asset.h
    nmos/authorization.h
    nmos/authorization_handlers.h
    nmos/authorization_redirect_api.h
    nmos/authorization_behaviour.h
    nmos/authorization_operation.h
    nmos/authorization_scopes.h
    nmos/authorization_state.h
    nmos/authorization_utils.h
    nmos/capabilities.h
    nmos/certificate_handlers.h
    nmos/certificate_settings.h
    nmos/channelmapping_activation.h
    nmos/channelmapping_api.h
    nmos/channelmapping_resources.h
    nmos/channels.h
    nmos/client_utils.h
    nmos/clock_name.h
    nmos/clock_ref_type.h
    nmos/colorspace.h
    nmos/components.h
    nmos/copyable_atomic.h
    nmos/configuration_api.h
    nmos/configuration_handlers.h
    nmos/configuration_methods.h
	nmos/configuration_resources.h
    nmos/configuration_utils.h
    nmos/connection_activation.h
    nmos/connection_api.h
    nmos/connection_events_activation.h
    nmos/connection_resources.h
    nmos/control_protocol_handlers.h
    nmos/control_protocol_methods.h
    nmos/control_protocol_nmos_channel_mapping_resource_type.h
    nmos/control_protocol_nmos_resource_type.h
    nmos/control_protocol_resource.h
    nmos/control_protocol_resources.h
    nmos/control_protocol_state.h
    nmos/control_protocol_typedefs.h
    nmos/control_protocol_utils.h
    nmos/control_protocol_ws_api.h
    nmos/device_type.h
    nmos/did_sdid.h
    nmos/event_type.h
    nmos/events_api.h
    nmos/events_resources.h
    nmos/events_ws_api.h
    nmos/events_ws_client.h
    nmos/filesystem_route.h
    nmos/format.h
    nmos/group_hint.h
    nmos/health.h
    nmos/id.h
    nmos/interlace_mode.h
    nmos/is04_versions.h
    nmos/is05_versions.h
    nmos/is07_versions.h
    nmos/is08_versions.h
    nmos/is09_versions.h
    nmos/is10_versions.h
    nmos/is12_versions.h
    nmos/is14_versions.h
    nmos/issuers.h
    nmos/json_fields.h
    nmos/json_schema.h
    nmos/jwks_uri_api.h
    nmos/jwk_utils.h
    nmos/jwt_generator.h
    nmos/jwt_validator.h
    nmos/lldp_handler.h
    nmos/lldp_manager.h
    nmos/log_gate.h
    nmos/log_manip.h
    nmos/log_model.h
    nmos/logging_api.h
    nmos/manifest_api.h
    nmos/mdns.h
    nmos/mdns_api.h
    nmos/mdns_versions.h
    nmos/media_type.h
    nmos/model.h
    nmos/mutex.h
    nmos/node_api.h
    nmos/node_api_target_handler.h
    nmos/node_behaviour.h
    nmos/node_interfaces.h
    nmos/node_resource.h
    nmos/node_resources.h
    nmos/node_server.h
    nmos/node_system_behaviour.h
    nmos/ocsp_behaviour.h
    nmos/ocsp_response_handler.h
    nmos/ocsp_state.h
    nmos/ocsp_utils.h
    nmos/paging_utils.h
    nmos/process_utils.h
    nmos/query_api.h
    nmos/query_utils.h
    nmos/query_ws_api.h
    nmos/random.h
    nmos/rational.h
    nmos/registration_api.h
    nmos/registry_resources.h
    nmos/registry_server.h
    nmos/resource.h
    nmos/resources.h
    nmos/schemas_api.h
    nmos/scope.h
    nmos/sdp_attributes.h
    nmos/sdp_utils.h
    nmos/server.h
    nmos/server_utils.h
    nmos/settings.h
    nmos/settings_api.h
    nmos/slog.h
    nmos/ssl_context_options.h
    nmos/st2110_21_sender_type.h
    nmos/string_enum.h
    nmos/string_enum_fwd.h
    nmos/system_api.h
    nmos/system_resources.h
    nmos/tai.h
    nmos/thread_utils.h
    nmos/transfer_characteristic.h
    nmos/transport.h
    nmos/type.h
    nmos/version.h
    nmos/video_jxsv.h
    nmos/vpid_code.h
    nmos/websockets.h
    nmos/ws_api_utils.h
    )

set(NMOS_CPP_PPLX_SOURCES
    pplx/pplx_utils.cpp
    )
set(NMOS_CPP_PPLX_HEADERS
    pplx/pplx_utils.h
    )

set(NMOS_CPP_RQL_SOURCES
    rql/rql.cpp
    )
set(NMOS_CPP_RQL_HEADERS
    rql/rql.h
    )

set(NMOS_CPP_SDP_SOURCES
    sdp/sdp_grammar.cpp
    )
set(NMOS_CPP_SDP_HEADERS
    sdp/json.h
    sdp/ntp.h
    sdp/sdp.h
    sdp/sdp_grammar.h
    )

set(NMOS_CPP_SSL_SOURCES
    ssl/ssl_utils.cpp
    )
set(NMOS_CPP_SSL_HEADERS
    ssl/ssl_utils.h
    )

add_library(
    nmos-cpp STATIC
    ${NMOS_CPP_BST_SOURCES}
    ${NMOS_CPP_BST_HEADERS}
    ${NMOS_CPP_CPPREST_SOURCES}
    ${NMOS_CPP_CPPREST_HEADERS}
    ${NMOS_CPP_NMOS_SOURCES}
    ${NMOS_CPP_NMOS_HEADERS}
    ${NMOS_CPP_JWK_HEADERS}
    ${NMOS_CPP_PPLX_SOURCES}
    ${NMOS_CPP_PPLX_HEADERS}
    ${NMOS_CPP_RQL_SOURCES}
    ${NMOS_CPP_RQL_HEADERS}
    ${NMOS_CPP_SDP_SOURCES}
    ${NMOS_CPP_SDP_HEADERS}
    ${NMOS_CPP_SSL_SOURCES}
    ${NMOS_CPP_SSL_HEADERS}
    )

source_group("bst\\Source Files" FILES ${NMOS_CPP_BST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_NMOS_SOURCES})
source_group("pplx\\Source Files" FILES ${NMOS_CPP_PPLX_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_RQL_SOURCES})
source_group("sdp\\Source Files" FILES ${NMOS_CPP_SDP_SOURCES})
source_group("ssl\\Source Files" FILES ${NMOS_CPP_SSL_SOURCES})

source_group("bst\\Header Files" FILES ${NMOS_CPP_BST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_CPPREST_HEADERS})
source_group("jwk\\Header Files" FILES ${NMOS_CPP_JWK_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_NMOS_HEADERS})
source_group("pplx\\Header Files" FILES ${NMOS_CPP_PPLX_HEADERS})
source_group("rql\\Header Files" FILES ${NMOS_CPP_RQL_HEADERS})
source_group("sdp\\Header Files" FILES ${NMOS_CPP_SDP_HEADERS})
source_group("ssl\\Header Files" FILES ${NMOS_CPP_SSL_HEADERS})

target_link_libraries(
    nmos-cpp PRIVATE
    nmos-cpp::compile-settings
    )
target_link_libraries(
    nmos-cpp PUBLIC
    nmos-cpp::nmos_is04_schemas
    nmos-cpp::nmos_is05_schemas
    nmos-cpp::nmos_is08_schemas
    nmos-cpp::nmos_is09_schemas
    nmos-cpp::nmos_is10_schemas
    nmos-cpp::nmos_is12_schemas
    nmos-cpp::nmos_is14_schemas
    nmos-cpp::mdns
    nmos-cpp::slog
    nmos-cpp::OpenSSL
    nmos-cpp::cpprestsdk
    nmos-cpp::Boost
    nmos-cpp::jwt-cpp
    )
target_link_libraries(
    nmos-cpp PRIVATE
    nmos-cpp::websocketpp
    nmos-cpp::json_schema_validator
    )
if(NMOS_CPP_BUILD_LLDP)
    target_link_libraries(
        nmos-cpp PUBLIC
        nmos-cpp::lldp
        )
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # link to resolver functions (for cpprest/host_utils.cpp)
    # note: this is no longer required on all platforms
    target_link_libraries(
        nmos-cpp PUBLIC
        resolv
        )
    if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.3)
        # link to std::filesystem functions (for bst/filesystem.h, used by nmos/filesystem_route.cpp)
        target_link_libraries(
            nmos-cpp PUBLIC
            stdc++fs
            )
    endif()
endif()
target_include_directories(nmos-cpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>
    )

install(FILES ${NMOS_CPP_BST_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/bst)
install(FILES ${NMOS_CPP_CPPREST_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/cpprest)
install(FILES ${NMOS_CPP_CPPREST_DETAILS_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/cpprest/details)
install(FILES ${NMOS_CPP_JWK_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/jwk)
install(FILES ${NMOS_CPP_NMOS_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/nmos)
install(FILES ${NMOS_CPP_PPLX_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/pplx)
install(FILES ${NMOS_CPP_RQL_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/rql)
install(FILES ${NMOS_CPP_SDP_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/sdp)
install(FILES ${NMOS_CPP_SSL_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/ssl)

list(APPEND NMOS_CPP_TARGETS nmos-cpp)
add_library(nmos-cpp::nmos-cpp ALIAS nmos-cpp)
