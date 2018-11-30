# Cmake instructions for making all the nmos-cpp libaries

# caller can set NMOS_HOME_DIR if the project is differend
if (NOT DEFINED NMOS_HOME_DIR)
    set (NMOS_HOME_DIR ${PROJECT_SOURCE_DIR})
endif()

# mdns library

set(MDNS_SOURCES
    ${NMOS_HOME_DIR}/mdns/core.cpp
    ${NMOS_HOME_DIR}/mdns/dns_sd_impl.cpp
    ${NMOS_HOME_DIR}/mdns/service_advertiser_impl.cpp
    ${NMOS_HOME_DIR}/mdns/service_discovery_impl.cpp
    )
set(MDNS_HEADERS
    ${NMOS_HOME_DIR}/mdns/core.h
    ${NMOS_HOME_DIR}/mdns/dns_sd_impl.h
    ${NMOS_HOME_DIR}/mdns/service_advertiser.h
    ${NMOS_HOME_DIR}/mdns/service_discovery.h
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
    ${NMOS_HOME_DIR}/nmos/is04_schemas/is04_schemas.h
    )

set(NMOS_IS04_V1_2_TAG v1.2.1)
set(NMOS_IS04_V1_1_TAG v1.1.2)
set(NMOS_IS04_V1_0_TAG v1.0.3)

set(NMOS_IS04_V1_2_SCHEMAS_JSON
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/clock_internal.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/clock_ptp.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/device.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/devices.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/error.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flows.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio_coded.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio_raw.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_data.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_mux.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_sdianc_data.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video_coded.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video_raw.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/node.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodes.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receivers.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_audio.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_data.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_mux.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_video.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/resource_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/sender.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/senders.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/sources.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_audio.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_1_SCHEMAS_JSON
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/clock_internal.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/clock_ptp.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/device.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/devices.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/error.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flows.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio_coded.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio_raw.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_data.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_mux.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_sdianc_data.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video_coded.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video_raw.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/node.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodes.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receivers.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_audio.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_data.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_mux.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_video.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/resource_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/sender.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/senders.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/sources.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_audio.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_core.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_0_SCHEMAS_JSON
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/device.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/devices.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/error.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/flow.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/flows.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/node.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodes.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-v1.0-subscriptions-post-request.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-v1.0-subscriptions-websocket.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/receiver.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/receivers.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-v1.0-resource-post-request.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sender-target.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sender.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/senders.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/source.json
    ${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sources.json
    )

set(NMOS_IS04_SCHEMAS_JSON_MATCH "${NMOS_HOME_DIR}/third_party/nmos-discovery-registration/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS04_SCHEMAS_SOURCE_REPLACE "${CMAKE_BINARY_DIR}/nmos/is04_schemas/\\1/\\2.cpp")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_2_SCHEMAS_SOURCES "${NMOS_IS04_V1_2_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_1_SCHEMAS_SOURCES "${NMOS_IS04_V1_1_SCHEMAS_JSON}")
string(REGEX REPLACE "${NMOS_IS04_SCHEMAS_JSON_MATCH}(;|$)" "${NMOS_IS04_SCHEMAS_SOURCE_REPLACE}\\3" NMOS_IS04_V1_0_SCHEMAS_SOURCES "${NMOS_IS04_V1_0_SCHEMAS_JSON}")

foreach(JSON ${NMOS_IS04_V1_2_SCHEMAS_JSON} ${NMOS_IS04_V1_1_SCHEMAS_JSON} ${NMOS_IS04_V1_0_SCHEMAS_JSON})
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
    ${NMOS_IS04_V1_1_SCHEMAS_SOURCES}
    ${NMOS_IS04_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is04_schemas\\Header Files" FILES ${NMOS_IS04_SCHEMAS_HEADERS})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_2_TAG}\\Source Files" FILES ${NMOS_IS04_V1_2_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_1_TAG}\\Source Files" FILES ${NMOS_IS04_V1_1_SCHEMAS_SOURCES})
source_group("nmos\\is04_schemas\\${NMOS_IS04_V1_0_TAG}\\Source Files" FILES ${NMOS_IS04_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is04_schemas_static
    )

# json schema validator library

set(JSON_SCHEMA_VALIDATOR_SOURCES
    ${NMOS_HOME_DIR}/third_party/nlohmann/json-validator.cpp
    ${NMOS_HOME_DIR}/third_party/nlohmann/json-uri.cpp
    )

set(JSON_SCHEMA_VALIDATOR_HEADERS
    ${NMOS_HOME_DIR}/third_party/nlohmann/json-schema.hpp
    ${NMOS_HOME_DIR}/third_party/nlohmann/json.hpp
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

# nmos_cpp library

#set(NMOS_CPP_NODE_SOURCES
#    ${NMOS_HOME_DIR}/nmos-cpp-node/main.cpp
#    )
#set(NMOS_CPP_NODE_HEADERS
#    ${NMOS_HOME_DIR}/nmos-cpp-node/main_gate.h
#    )

set(NMOS_CPP_NODE_BST_SOURCES
    )
set(NMOS_CPP_NODE_BST_HEADERS
    ${NMOS_HOME_DIR}/bst/filesystem.h
    ${NMOS_HOME_DIR}/bst/regex.h
    ${NMOS_HOME_DIR}/bst/shared_mutex.h
    )

set(NMOS_CPP_NODE_CPPREST_SOURCES
    ${NMOS_HOME_DIR}/cpprest/api_router.cpp
    ${NMOS_HOME_DIR}/cpprest/host_utils.cpp
    ${NMOS_HOME_DIR}/cpprest/http_utils.cpp
    ${NMOS_HOME_DIR}/cpprest/json_utils.cpp
	${NMOS_HOME_DIR}/cpprest/json_validator_impl.cpp
    ${NMOS_HOME_DIR}/cpprest/ws_listener_impl.cpp
    )
set(NMOS_CPP_NODE_CPPREST_HEADERS
    ${NMOS_HOME_DIR}/cpprest/api_router.h
    ${NMOS_HOME_DIR}/cpprest/basic_utils.h
    ${NMOS_HOME_DIR}/cpprest/host_utils.h
    ${NMOS_HOME_DIR}/cpprest/http_utils.h
    ${NMOS_HOME_DIR}/cpprest/json_utils.h
	${NMOS_HOME_DIR}/cpprest/json_validator.h
    ${NMOS_HOME_DIR}/cpprest/logging_utils.h
    ${NMOS_HOME_DIR}/cpprest/regex_utils.h
    ${NMOS_HOME_DIR}/cpprest/ws_listener.h
    )

set(NMOS_CPP_NODE_NMOS_SOURCES
    ${NMOS_HOME_DIR}/nmos/admin_ui.cpp
    ${NMOS_HOME_DIR}/nmos/api_downgrade.cpp
    ${NMOS_HOME_DIR}/nmos/api_utils.cpp
    ${NMOS_HOME_DIR}/nmos/components.cpp
    ${NMOS_HOME_DIR}/nmos/connection_api.cpp
    ${NMOS_HOME_DIR}/nmos/filesystem_route.cpp
    ${NMOS_HOME_DIR}/nmos/group_hint.cpp
    ${NMOS_HOME_DIR}/nmos/id.cpp
    ${NMOS_HOME_DIR}/nmos/json_schema.cpp
    ${NMOS_HOME_DIR}/nmos/logging_api.cpp
    ${NMOS_HOME_DIR}/nmos/mdns.cpp
    ${NMOS_HOME_DIR}/nmos/mdns_api.cpp
    ${NMOS_HOME_DIR}/nmos/node_api.cpp
    ${NMOS_HOME_DIR}/nmos/node_api_target_handler.cpp
    ${NMOS_HOME_DIR}/nmos/node_behaviour.cpp
    ${NMOS_HOME_DIR}/nmos/node_resource.cpp
    ${NMOS_HOME_DIR}/nmos/node_resources.cpp
    ${NMOS_HOME_DIR}/nmos/node_resources_thread.cpp
    ${NMOS_HOME_DIR}/nmos/process_utils.cpp
    ${NMOS_HOME_DIR}/nmos/query_api.cpp
    ${NMOS_HOME_DIR}/nmos/query_utils.cpp
    ${NMOS_HOME_DIR}/nmos/query_ws_api.cpp
    ${NMOS_HOME_DIR}/nmos/registration_api.cpp
    ${NMOS_HOME_DIR}/nmos/registry_resources.cpp
    ${NMOS_HOME_DIR}/nmos/resource.cpp
    ${NMOS_HOME_DIR}/nmos/resources.cpp
    ${NMOS_HOME_DIR}/nmos/sdp_utils.cpp
    ${NMOS_HOME_DIR}/nmos/settings_api.cpp
    )
set(NMOS_CPP_NODE_NMOS_HEADERS
    ${NMOS_HOME_DIR}/nmos/activation_mode.h
    ${NMOS_HOME_DIR}/nmos/admin_ui.h
    ${NMOS_HOME_DIR}/nmos/api_downgrade.h
    ${NMOS_HOME_DIR}/nmos/api_utils.h
    ${NMOS_HOME_DIR}/nmos/api_version.h
    ${NMOS_HOME_DIR}/nmos/channels.h
    ${NMOS_HOME_DIR}/nmos/colorspace.h
    ${NMOS_HOME_DIR}/nmos/components.h
    ${NMOS_HOME_DIR}/nmos/copyable_atomic.h
    ${NMOS_HOME_DIR}/nmos/connection_api.h
    ${NMOS_HOME_DIR}/nmos/device_type.h
    ${NMOS_HOME_DIR}/nmos/filesystem_route.h
    ${NMOS_HOME_DIR}/nmos/format.h
    ${NMOS_HOME_DIR}/nmos/group_hint.h
    ${NMOS_HOME_DIR}/nmos/health.h
    ${NMOS_HOME_DIR}/nmos/id.h
    ${NMOS_HOME_DIR}/nmos/interlace_mode.h
    ${NMOS_HOME_DIR}/nmos/json_fields.h
    ${NMOS_HOME_DIR}/nmos/json_schema.h
    ${NMOS_HOME_DIR}/nmos/log_manip.h
    ${NMOS_HOME_DIR}/nmos/logging_api.h
    ${NMOS_HOME_DIR}/nmos/mdns.h
    ${NMOS_HOME_DIR}/nmos/mdns_api.h
    ${NMOS_HOME_DIR}/nmos/media_type.h
    ${NMOS_HOME_DIR}/nmos/model.h
    ${NMOS_HOME_DIR}/nmos/mutex.h
    ${NMOS_HOME_DIR}/nmos/node_api.h
    ${NMOS_HOME_DIR}/nmos/node_api_target_handler.h
    ${NMOS_HOME_DIR}/nmos/node_behaviour.h
    ${NMOS_HOME_DIR}/nmos/node_resource.h
    ${NMOS_HOME_DIR}/nmos/node_resources.h
    ${NMOS_HOME_DIR}/nmos/node_resources_thread.h
    ${NMOS_HOME_DIR}/nmos/paging_utils.h
    ${NMOS_HOME_DIR}/nmos/process_utils.h
    ${NMOS_HOME_DIR}/nmos/query_api.h
    ${NMOS_HOME_DIR}/nmos/query_utils.h
    ${NMOS_HOME_DIR}/nmos/query_ws_api.h
    ${NMOS_HOME_DIR}/nmos/random.h
    ${NMOS_HOME_DIR}/nmos/rational.h
    ${NMOS_HOME_DIR}/nmos/registration_api.h
    ${NMOS_HOME_DIR}/nmos/registry_resources.h
    ${NMOS_HOME_DIR}/nmos/resource.h
    ${NMOS_HOME_DIR}/nmos/resources.h
    ${NMOS_HOME_DIR}/nmos/sdp_utils.h
    ${NMOS_HOME_DIR}/nmos/settings.h
    ${NMOS_HOME_DIR}/nmos/settings_api.h
    ${NMOS_HOME_DIR}/nmos/slog.h
    ${NMOS_HOME_DIR}/nmos/string_enum.h
    ${NMOS_HOME_DIR}/nmos/tai.h
    ${NMOS_HOME_DIR}/nmos/thread_utils.h
    ${NMOS_HOME_DIR}/nmos/transfer_characteristic.h
    ${NMOS_HOME_DIR}/nmos/transport.h
    ${NMOS_HOME_DIR}/nmos/type.h
    ${NMOS_HOME_DIR}/nmos/version.h
    )

set(NMOS_CPP_NODE_PPLX_SOURCES
    ${NMOS_HOME_DIR}/pplx/pplx_utils.cpp
    )
set(NMOS_CPP_NODE_PPLX_HEADERS
    ${NMOS_HOME_DIR}/pplx/pplx_utils.h
    )

set(NMOS_CPP_NODE_RQL_SOURCES
    ${NMOS_HOME_DIR}/rql/rql.cpp
    )
set(NMOS_CPP_NODE_RQL_HEADERS
    ${NMOS_HOME_DIR}/rql/rql.h
    )

set(NMOS_CPP_NODE_SDP_SOURCES
    ${NMOS_HOME_DIR}/sdp/sdp_grammar.cpp
    )
set(NMOS_CPP_NODE_SDP_HEADERS
    ${NMOS_HOME_DIR}/sdp/json.h
    ${NMOS_HOME_DIR}/sdp/sdp.h
    ${NMOS_HOME_DIR}/sdp/sdp_grammar.h
    )

set(NMOS_CPP_NODE_SLOG_HEADERS
    ${NMOS_HOME_DIR}/slog/all_in_one.h
    )

add_library(
    nmos_cpp_static STATIC
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
    ${NMOS_CPP_NODE_SDP_SOURCES}
    ${NMOS_CPP_NODE_SDP_HEADERS}
    ${NMOS_CPP_NODE_SLOG_HEADERS}
    )

#source_group("Source Files" FILES ${NMOS_CPP_NODE_SOURCES})
source_group("bst\\Source Files" FILES ${NMOS_CPP_NODE_BST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_NODE_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_NODE_NMOS_SOURCES})
source_group("pplx\\Source Files" FILES ${NMOS_CPP_NODE_PPLX_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_NODE_RQL_SOURCES})
source_group("sdp\\Source Files" FILES ${NMOS_CPP_NODE_SDP_SOURCES})

#source_group("Header Files" FILES ${NMOS_CPP_NODE_HEADERS})
source_group("bst\\Header Files" FILES ${NMOS_CPP_NODE_BST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_NODE_CPPREST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_NODE_NMOS_HEADERS})
source_group("pplx\\Header Files" FILES ${NMOS_CPP_NODE_PPLX_HEADERS})
source_group("rql\\Header Files" FILES ${NMOS_CPP_NODE_RQL_HEADERS})
source_group("sdp\\Header Files" FILES ${NMOS_CPP_NODE_SDP_HEADERS})
source_group("slog\\Header Files" FILES ${NMOS_CPP_NODE_SLOG_HEADERS})

target_link_libraries(
    nmos_cpp_static
    mdns_static
    cpprestsdk::cpprest
    json_schema_validator_static
	nmos_is04_schemas_static
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

## nmos-cpp-registry library
#
##set(NMOS_CPP_REGISTRY_SOURCES
##    ${NMOS_HOME_DIR}/nmos-cpp-registry/main.cpp
##    )
##set(NMOS_CPP_REGISTRY_HEADERS
##    ${NMOS_HOME_DIR}/nmos-cpp-registry/main_gate.h
##    )
#
#set(NMOS_CPP_REGISTRY_BST_SOURCES
#    )
#set(NMOS_CPP_REGISTRY_BST_HEADERS
#    ${NMOS_HOME_DIR}/bst/filesystem.h
#    ${NMOS_HOME_DIR}/bst/regex.h
#    ${NMOS_HOME_DIR}/bst/shared_mutex.h
#    )
#
#set(NMOS_CPP_REGISTRY_CPPREST_SOURCES
#    ${NMOS_HOME_DIR}/cpprest/api_router.cpp
#    ${NMOS_HOME_DIR}/cpprest/host_utils.cpp
#    ${NMOS_HOME_DIR}/cpprest/http_utils.cpp
#    ${NMOS_HOME_DIR}/cpprest/json_utils.cpp
#	${NMOS_HOME_DIR}/cpprest/json_validator_impl.cpp
#    ${NMOS_HOME_DIR}/cpprest/ws_listener_impl.cpp
#    )
#set(NMOS_CPP_REGISTRY_CPPREST_HEADERS
#    ${NMOS_HOME_DIR}/cpprest/api_router.h
#    ${NMOS_HOME_DIR}/cpprest/basic_utils.h
#    ${NMOS_HOME_DIR}/cpprest/host_utils.h
#    ${NMOS_HOME_DIR}/cpprest/http_utils.h
#    ${NMOS_HOME_DIR}/cpprest/json_utils.h
#	${NMOS_HOME_DIR}/cpprest/json_validator.h
#    ${NMOS_HOME_DIR}/cpprest/logging_utils.h
#    ${NMOS_HOME_DIR}/cpprest/regex_utils.h
#    ${NMOS_HOME_DIR}/cpprest/ws_listener.h
#    )
#
#set(NMOS_CPP_REGISTRY_NMOS_SOURCES
#    ${NMOS_HOME_DIR}/nmos/admin_ui.cpp
#    ${NMOS_HOME_DIR}/nmos/api_downgrade.cpp
#    ${NMOS_HOME_DIR}/nmos/api_utils.cpp
#    ${NMOS_HOME_DIR}/nmos/filesystem_route.cpp
#    ${NMOS_HOME_DIR}/nmos/id.cpp
#    ${NMOS_HOME_DIR}/nmos/json_schema.cpp
#    ${NMOS_HOME_DIR}/nmos/logging_api.cpp
#    ${NMOS_HOME_DIR}/nmos/mdns.cpp
#    ${NMOS_HOME_DIR}/nmos/mdns_api.cpp
#    ${NMOS_HOME_DIR}/nmos/node_api.cpp
#    ${NMOS_HOME_DIR}/nmos/node_resource.cpp
#    ${NMOS_HOME_DIR}/nmos/process_utils.cpp
#    ${NMOS_HOME_DIR}/nmos/query_api.cpp
#    ${NMOS_HOME_DIR}/nmos/query_utils.cpp
#    ${NMOS_HOME_DIR}/nmos/query_ws_api.cpp
#    ${NMOS_HOME_DIR}/nmos/registration_api.cpp
#    ${NMOS_HOME_DIR}/nmos/registry_resources.cpp
#    ${NMOS_HOME_DIR}/nmos/resource.cpp
#    ${NMOS_HOME_DIR}/nmos/resources.cpp
#    ${NMOS_HOME_DIR}/nmos/settings_api.cpp
#    )
#set(NMOS_CPP_REGISTRY_NMOS_HEADERS
#    ${NMOS_HOME_DIR}/nmos/activation_mode.h
#    ${NMOS_HOME_DIR}/nmos/admin_ui.h
#    ${NMOS_HOME_DIR}/nmos/api_downgrade.h
#    ${NMOS_HOME_DIR}/nmos/api_utils.h
#    ${NMOS_HOME_DIR}/nmos/api_version.h
#    ${NMOS_HOME_DIR}/nmos/copyable_atomic.h
#    ${NMOS_HOME_DIR}/nmos/filesystem_route.h
#    ${NMOS_HOME_DIR}/nmos/health.h
#    ${NMOS_HOME_DIR}/nmos/id.h
#    ${NMOS_HOME_DIR}/nmos/json_fields.h
#    ${NMOS_HOME_DIR}/nmos/json_schema.h
#    ${NMOS_HOME_DIR}/nmos/log_manip.h
#    ${NMOS_HOME_DIR}/nmos/logging_api.h
#    ${NMOS_HOME_DIR}/nmos/mdns.h
#    ${NMOS_HOME_DIR}/nmos/mdns_api.h
#    ${NMOS_HOME_DIR}/nmos/model.h
#    ${NMOS_HOME_DIR}/nmos/mutex.h
#    ${NMOS_HOME_DIR}/nmos/node_api.h
#    ${NMOS_HOME_DIR}/nmos/node_api_target_handler.h
#    ${NMOS_HOME_DIR}/nmos/node_resource.h
#    ${NMOS_HOME_DIR}/nmos/paging_utils.h
#    ${NMOS_HOME_DIR}/nmos/process_utils.h
#    ${NMOS_HOME_DIR}/nmos/query_api.h
#    ${NMOS_HOME_DIR}/nmos/query_utils.h
#    ${NMOS_HOME_DIR}/nmos/query_ws_api.h
#    ${NMOS_HOME_DIR}/nmos/random.h
#    ${NMOS_HOME_DIR}/nmos/rational.h
#    ${NMOS_HOME_DIR}/nmos/registration_api.h
#    ${NMOS_HOME_DIR}/nmos/registry_resources.h
#    ${NMOS_HOME_DIR}/nmos/resource.h
#    ${NMOS_HOME_DIR}/nmos/resources.h
#    ${NMOS_HOME_DIR}/nmos/settings.h
#    ${NMOS_HOME_DIR}/nmos/settings_api.h
#    ${NMOS_HOME_DIR}/nmos/slog.h
#    ${NMOS_HOME_DIR}/nmos/string_enum.h
#    ${NMOS_HOME_DIR}/nmos/tai.h
#    ${NMOS_HOME_DIR}/nmos/thread_utils.h
#    ${NMOS_HOME_DIR}/nmos/type.h
#    ${NMOS_HOME_DIR}/nmos/version.h
#    )
#
#set(NMOS_CPP_REGISTRY_RQL_SOURCES
#    ${NMOS_HOME_DIR}/rql/rql.cpp
#    )
#set(NMOS_CPP_REGISTRY_RQL_HEADERS
#    ${NMOS_HOME_DIR}/rql/rql.h
#    )
#
#set(NMOS_CPP_REGISTRY_SLOG_HEADERS
#    ${NMOS_HOME_DIR}/slog/all_in_one.h
#    )
#
#add_library(
#    nmos-cpp-registry_static STATIC
#    ${NMOS_CPP_REGISTRY_BST_SOURCES}
#    ${NMOS_CPP_REGISTRY_BST_HEADERS}
#    ${NMOS_CPP_REGISTRY_CPPREST_SOURCES}
#    ${NMOS_CPP_REGISTRY_CPPREST_HEADERS}
#    ${NMOS_CPP_REGISTRY_NMOS_SOURCES}
#    ${NMOS_CPP_REGISTRY_NMOS_HEADERS}
#    ${NMOS_CPP_REGISTRY_RQL_SOURCES}
#    ${NMOS_CPP_REGISTRY_RQL_HEADERS}
#    ${NMOS_CPP_REGISTRY_SLOG_HEADERS}
#    )
#
##source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_SOURCES})
#source_group("bst\\Source Files" FILES ${NMOS_CPP_REGISTRY_BST_SOURCES})
#source_group("cpprest\\Source Files" FILES ${NMOS_CPP_REGISTRY_CPPREST_SOURCES})
#source_group("nmos\\Source Files" FILES ${NMOS_CPP_REGISTRY_NMOS_SOURCES})
#source_group("rql\\Source Files" FILES ${NMOS_CPP_REGISTRY_RQL_SOURCES})
#
##source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_HEADERS})
#source_group("bst\\Header Files" FILES ${NMOS_CPP_REGISTRY_BST_HEADERS})
#source_group("cpprest\\Header Files" FILES ${NMOS_CPP_REGISTRY_CPPREST_HEADERS})
#source_group("nmos\\Header Files" FILES ${NMOS_CPP_REGISTRY_NMOS_HEADERS})
#source_group("rql\\Header Files" FILES ${NMOS_CPP_REGISTRY_RQL_HEADERS})
#source_group("slog\\Header Files" FILES ${NMOS_CPP_REGISTRY_SLOG_HEADERS})
#
#target_link_libraries(
#    nmos-cpp-registry_static
#    mdns_static
#    cpprestsdk::cpprest
#	json_schema_validator_static
#	nmos_is04_schemas_static
#    ${BONJOUR_LIB}
#    ${PLATFORM_LIBS}
#    ${Boost_LIBRARIES}
#    )
#

# nmos-cpp-registry-test library

#set(NMOS_CPP_REGISTRY_TEST_SOURCES
#    ${NMOS_HOME_DIR}/nmos-cpp-registry/test/main.cpp
#    )
#set(NMOS_CPP_REGISTRY_TEST_HEADERS
#    )

set(NMOS_CPP_REGISTRY_TEST_BST_TEST_SOURCES
    )
set(NMOS_CPP_REGISTRY_TEST_BST_TEST_HEADERS
    ${NMOS_HOME_DIR}/bst/test/test.h
    )

set(NMOS_CPP_REGISTRY_TEST_CPPREST_SOURCES
    ${NMOS_HOME_DIR}/cpprest/api_router.cpp
    ${NMOS_HOME_DIR}/cpprest/host_utils.cpp
    ${NMOS_HOME_DIR}/cpprest/http_utils.cpp
    ${NMOS_HOME_DIR}/cpprest/json_utils.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_CPPREST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_SOURCES
    ${NMOS_HOME_DIR}/cpprest/test/api_router_test.cpp
    ${NMOS_HOME_DIR}/cpprest/test/http_utils_test.cpp
    ${NMOS_HOME_DIR}/cpprest/test/json_utils_test.cpp
    ${NMOS_HOME_DIR}/cpprest/test/regex_utils_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_MDNS_TEST_SOURCES
    ${NMOS_HOME_DIR}/mdns/test/core_test.cpp
    ${NMOS_HOME_DIR}/mdns/test/mdns_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_MDNS_TEST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_NMOS_SOURCES
    ${NMOS_HOME_DIR}/nmos/api_utils.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_NMOS_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_NMOS_TEST_SOURCES
    ${NMOS_HOME_DIR}/nmos/test/api_utils_test.cpp
    ${NMOS_HOME_DIR}/nmos/test/paging_utils_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_NMOS_TEST_HEADERS
    )

set(NMOS_CPP_REGISTRY_TEST_SDP_SOURCES
    ${NMOS_HOME_DIR}/sdp/sdp_grammar.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_SDP_HEADERS
    ${NMOS_HOME_DIR}/sdp/json.h
    ${NMOS_HOME_DIR}/sdp/sdp.h
    ${NMOS_HOME_DIR}/sdp/sdp_grammar.h
    )

set(NMOS_CPP_REGISTRY_TEST_SDP_TEST_SOURCES
    ${NMOS_HOME_DIR}/sdp/test/sdp_test.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_SDP_TEST_HEADERS
    )

add_library(
    nmos_cpp_registry_test_static STATIC
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
    ${NMOS_CPP_REGISTRY_TEST_SDP_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_SDP_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_HEADERS}
    )

#source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SOURCES})
source_group("bst\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_BST_TEST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_SOURCES})
source_group("cpprest\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_SOURCES})
source_group("mdns\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_SOURCES})
source_group("nmos\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_SOURCES})
source_group("sdp\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_SOURCES})
source_group("sdp\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_SOURCES})

#source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_HEADERS})
source_group("bst\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_BST_TEST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_HEADERS})
source_group("cpprest\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_HEADERS})
source_group("mdns\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_HEADERS})
source_group("nmos\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_HEADERS})
source_group("sdp\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_HEADERS})
source_group("sdp\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_HEADERS})

target_link_libraries(
    nmos_cpp_registry_test_static
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

#include(Catch)

#catch_discover_tests(nmos-cpp-registry-test EXTRA_ARGS -r compact)
