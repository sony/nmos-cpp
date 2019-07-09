# CMake instructions for making all the nmos-cpp libraries

# caller can set NMOS_CPP_DIR if the project is different
if (NOT DEFINED NMOS_CPP_DIR)
    set (NMOS_CPP_DIR ${PROJECT_SOURCE_DIR})
endif()

# detail headers

set(DETAIL_HEADERS
    ${NMOS_CPP_DIR}/detail/for_each_reversed.h
    ${NMOS_CPP_DIR}/detail/pragma_warnings.h
    ${NMOS_CPP_DIR}/detail/private_access.h
    )

if(MSVC)
    list(APPEND DETAIL_HEADERS
        ${NMOS_CPP_DIR}/detail/vc_disable_dll_warnings.h
        ${NMOS_CPP_DIR}/detail/vc_disable_warnings.h
        )
endif()

install(FILES ${DETAIL_HEADERS} DESTINATION include/detail)

# mdns library

set(MDNS_SOURCES
    ${NMOS_CPP_DIR}/mdns/core.cpp
    ${NMOS_CPP_DIR}/mdns/dns_sd_impl.cpp
    ${NMOS_CPP_DIR}/mdns/service_advertiser_impl.cpp
    ${NMOS_CPP_DIR}/mdns/service_discovery_impl.cpp
    )
set(MDNS_HEADERS
    ${NMOS_CPP_DIR}/mdns/core.h
    ${NMOS_CPP_DIR}/mdns/dns_sd_impl.h
    ${NMOS_CPP_DIR}/mdns/service_advertiser.h
    ${NMOS_CPP_DIR}/mdns/service_discovery.h
    )

add_library(
    mdns_static STATIC
    ${MDNS_SOURCES}
    ${MDNS_HEADERS}
    ${BONJOUR_SOURCES}
    ${BONJOUR_HEADERS}
    )

source_group("Source Files" FILES ${MDNS_SOURCES} ${BONJOUR_SOURCES})
source_group("Header Files" FILES ${MDNS_HEADERS} ${BONJOUR_HEADERS})

# ensure e.g. target_compile_definitions for cppprestsdk::cpprest are applied when building this target
target_link_libraries(
    mdns_static
    cpprestsdk::cpprest
    )

install(TARGETS mdns_static DESTINATION lib)
install(FILES ${MDNS_HEADERS} DESTINATION include/mdns)

# nmos_is04_schemas library

set(NMOS_IS04_SCHEMAS_HEADERS
    ${NMOS_CPP_DIR}/nmos/is04_schemas/is04_schemas.h
    )

set(NMOS_IS04_V1_3_TAG v1.3-dev)
set(NMOS_IS04_V1_2_TAG v1.2.2)
set(NMOS_IS04_V1_1_TAG v1.1.3)
set(NMOS_IS04_V1_0_TAG v1.0.3)

set(NMOS_IS04_V1_3_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/clock_internal.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/clock_ptp.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/device.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/devices.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flows.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_audio_coded.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_audio_raw.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_json_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_mux.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_sdianc_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_video.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_video_coded.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/flow_video_raw.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/node.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/nodes.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receivers.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_mux.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/receiver_video.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/resource_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/sender.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/senders.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/sources.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_3_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_2_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/clock_internal.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/clock_ptp.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/device.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/devices.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flows.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio_coded.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_audio_raw.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_mux.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_sdianc_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video_coded.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/flow_video_raw.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/node.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/nodes.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receivers.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_mux.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/receiver_video.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/resource_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/sender.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/senders.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/sources.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_2_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_1_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/clock_internal.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/clock_ptp.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/device.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/devices.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flows.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio_coded.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_audio_raw.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_mux.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_sdianc_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video_coded.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/flow_video_raw.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/node.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/nodes.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/queryapi-subscriptions-websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receivers.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_data.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_mux.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/receiver_video.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-resource-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/resource_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/sender.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/senders.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/sources.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_audio.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_core.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_1_TAG}/APIs/schemas/source_generic.json
    )

set(NMOS_IS04_V1_0_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/device.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/devices.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/flow.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/flows.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/node.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodeapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodeapi-receiver-target.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/nodes.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-subscription-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-subscriptions-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-v1.0-subscriptions-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/queryapi-v1.0-subscriptions-websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/receiver.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/receivers.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-health-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-resource-response.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/registrationapi-v1.0-resource-post-request.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sender-target.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sender.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/senders.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/source.json
    ${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/${NMOS_IS04_V1_0_TAG}/APIs/schemas/sources.json
    )

set(NMOS_IS04_SCHEMAS_JSON_MATCH "${NMOS_CPP_DIR}/third_party/nmos-discovery-registration/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS04_SCHEMAS_SOURCE_REPLACE "${CMAKE_BINARY_DIR}/nmos/is04_schemas/\\1/\\2.cpp")
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
    nmos_is04_schemas_static STATIC
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
    nmos_is04_schemas_static
    )

install(TARGETS nmos_is04_schemas_static DESTINATION lib)

# nmos_is05_schemas library

set(NMOS_IS05_SCHEMAS_HEADERS
    ${NMOS_CPP_DIR}/nmos/is05_schemas/is05_schemas.h
    )

set(NMOS_IS05_V1_1_TAG v1.1-dev)
set(NMOS_IS05_V1_0_TAG v1.0.2)

set(NMOS_IS05_V1_1_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/activation-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/activation-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/bulk-receiver-post-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/bulk-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/bulk-sender-post-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-bulk.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-receiver.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-sender.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/connectionapi-single.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraint-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema-mqtt.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema-rtp.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema-websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/constraints-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_dash.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_ext.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_mqtt.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_rtp.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver_transport_params_websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver-stage-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/receiver-transport-file.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_dash.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_ext.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_mqtt.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_rtp.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender_transport_params_websocket.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender-receiver-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/sender-stage-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_1_TAG}/APIs/schemas/transporttype-response-schema.json
    )

set(NMOS_IS05_V1_0_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-bulk.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-receiver.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-sender.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/connectionapi-single.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_receiver_transport_params_dash.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_receiver_transport_params_rtp.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_sender_transport_params_dash.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0_sender_transport_params_rtp.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-activation-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-activation-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-bulk-receiver-post-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-bulk-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-bulk-sender-post-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-constraints-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-receiver-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-receiver-stage-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/sender-receiver-base.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-sender-response-schema.json
    ${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/${NMOS_IS05_V1_0_TAG}/APIs/schemas/v1.0-sender-stage-schema.json
    )

set(NMOS_IS05_SCHEMAS_JSON_MATCH "${NMOS_CPP_DIR}/third_party/nmos-device-connection-management/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS05_SCHEMAS_SOURCE_REPLACE "${CMAKE_BINARY_DIR}/nmos/is05_schemas/\\1/\\2.cpp")
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
    nmos_is05_schemas_static STATIC
    ${NMOS_IS05_SCHEMAS_HEADERS}
    ${NMOS_IS05_V1_1_SCHEMAS_SOURCES}
    ${NMOS_IS05_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is05_schemas\\Header Files" FILES ${NMOS_IS05_SCHEMAS_HEADERS})
source_group("nmos\\is05_schemas\\${NMOS_IS05_V1_1_TAG}\\Source Files" FILES ${NMOS_IS05_V1_1_SCHEMAS_SOURCES})
source_group("nmos\\is05_schemas\\${NMOS_IS05_V1_0_TAG}\\Source Files" FILES ${NMOS_IS05_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is05_schemas_static
    )

install(TARGETS nmos_is05_schemas_static DESTINATION lib)

# nmos_is09_schemas library

set(NMOS_IS09_SCHEMAS_HEADERS
    ${NMOS_CPP_DIR}/nmos/is09_schemas/is09_schemas.h
    )

set(NMOS_IS09_V1_0_TAG v1.0-dev)

set(NMOS_IS09_V1_0_SCHEMAS_JSON
    ${NMOS_CPP_DIR}/third_party/nmos-system/${NMOS_IS09_V1_0_TAG}/APIs/schemas/base.json
    ${NMOS_CPP_DIR}/third_party/nmos-system/${NMOS_IS09_V1_0_TAG}/APIs/schemas/error.json
    ${NMOS_CPP_DIR}/third_party/nmos-system/${NMOS_IS09_V1_0_TAG}/APIs/schemas/global.json
    ${NMOS_CPP_DIR}/third_party/nmos-system/${NMOS_IS09_V1_0_TAG}/APIs/schemas/resource_core.json
    )

set(NMOS_IS09_SCHEMAS_JSON_MATCH "${NMOS_CPP_DIR}/third_party/nmos-system/([^/]+)/APIs/schemas/([^;]+)\\.json")
set(NMOS_IS09_SCHEMAS_SOURCE_REPLACE "${CMAKE_BINARY_DIR}/nmos/is09_schemas/\\1/\\2.cpp")
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
    nmos_is09_schemas_static STATIC
    ${NMOS_IS09_SCHEMAS_HEADERS}
    ${NMOS_IS09_V1_0_SCHEMAS_SOURCES}
    )

source_group("nmos\\is09_schemas\\Header Files" FILES ${NMOS_IS09_SCHEMAS_HEADERS})
source_group("nmos\\is09_schemas\\${NMOS_IS09_V1_0_TAG}\\Source Files" FILES ${NMOS_IS09_V1_0_SCHEMAS_SOURCES})

target_link_libraries(
    nmos_is09_schemas_static
    )

install(TARGETS nmos_is09_schemas_static DESTINATION lib)

# json schema validator library

set(JSON_SCHEMA_VALIDATOR_SOURCES
    ${NMOS_CPP_DIR}/third_party/nlohmann/json-schema-draft7.json.cpp
    ${NMOS_CPP_DIR}/third_party/nlohmann/json-validator.cpp
    ${NMOS_CPP_DIR}/third_party/nlohmann/json-uri.cpp
    )

set(JSON_SCHEMA_VALIDATOR_HEADERS
    ${NMOS_CPP_DIR}/third_party/nlohmann/json-schema.hpp
    ${NMOS_CPP_DIR}/third_party/nlohmann/json.hpp
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

install(TARGETS json_schema_validator_static DESTINATION lib)

# nmos-cpp library

set(NMOS_CPP_BST_SOURCES
    )
set(NMOS_CPP_BST_HEADERS
    ${NMOS_CPP_DIR}/bst/filesystem.h
    ${NMOS_CPP_DIR}/bst/regex.h
    ${NMOS_CPP_DIR}/bst/shared_mutex.h
    )

set(NMOS_CPP_CPPREST_SOURCES
    ${NMOS_CPP_DIR}/cpprest/api_router.cpp
    ${NMOS_CPP_DIR}/cpprest/host_utils.cpp
    ${NMOS_CPP_DIR}/cpprest/http_utils.cpp
    ${NMOS_CPP_DIR}/cpprest/json_escape.cpp
    ${NMOS_CPP_DIR}/cpprest/json_storage.cpp
    ${NMOS_CPP_DIR}/cpprest/json_utils.cpp
    ${NMOS_CPP_DIR}/cpprest/json_validator_impl.cpp
    ${NMOS_CPP_DIR}/cpprest/json_visit.cpp
    ${NMOS_CPP_DIR}/cpprest/ws_listener_impl.cpp
    )

if(MSVC)
    # workaround for "fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj"
    set_source_files_properties(${NMOS_CPP_DIR}/cpprest/ws_listener_impl.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

set(NMOS_CPP_CPPREST_HEADERS
    ${NMOS_CPP_DIR}/cpprest/api_router.h
    ${NMOS_CPP_DIR}/cpprest/basic_utils.h
    ${NMOS_CPP_DIR}/cpprest/host_utils.h
    ${NMOS_CPP_DIR}/cpprest/http_utils.h
    ${NMOS_CPP_DIR}/cpprest/json_escape.h
    ${NMOS_CPP_DIR}/cpprest/json_ops.h
    ${NMOS_CPP_DIR}/cpprest/json_storage.h
    ${NMOS_CPP_DIR}/cpprest/json_utils.h
    ${NMOS_CPP_DIR}/cpprest/json_validator.h
    ${NMOS_CPP_DIR}/cpprest/json_visit.h
    ${NMOS_CPP_DIR}/cpprest/logging_utils.h
    ${NMOS_CPP_DIR}/cpprest/regex_utils.h
    ${NMOS_CPP_DIR}/cpprest/uri_schemes.h
    ${NMOS_CPP_DIR}/cpprest/ws_listener.h
    ${NMOS_CPP_DIR}/cpprest/ws_utils.h
    )

set(NMOS_CPP_CPPREST_DETAILS_HEADERS
    ${NMOS_CPP_DIR}/cpprest/details/boost_u_workaround.h
    ${NMOS_CPP_DIR}/cpprest/details/pop_u.h
    ${NMOS_CPP_DIR}/cpprest/details/push_undef_u.h
    ${NMOS_CPP_DIR}/cpprest/details/system_error.h
    )

set(NMOS_CPP_NMOS_SOURCES
    ${NMOS_CPP_DIR}/nmos/admin_ui.cpp
    ${NMOS_CPP_DIR}/nmos/api_downgrade.cpp
    ${NMOS_CPP_DIR}/nmos/api_utils.cpp
    ${NMOS_CPP_DIR}/nmos/client_utils.cpp
    ${NMOS_CPP_DIR}/nmos/components.cpp
    ${NMOS_CPP_DIR}/nmos/connection_activation.cpp
    ${NMOS_CPP_DIR}/nmos/connection_api.cpp
    ${NMOS_CPP_DIR}/nmos/connection_resources.cpp
    ${NMOS_CPP_DIR}/nmos/events_api.cpp
    ${NMOS_CPP_DIR}/nmos/events_resources.cpp
    ${NMOS_CPP_DIR}/nmos/events_ws_api.cpp
    ${NMOS_CPP_DIR}/nmos/filesystem_route.cpp
    ${NMOS_CPP_DIR}/nmos/group_hint.cpp
    ${NMOS_CPP_DIR}/nmos/id.cpp
    ${NMOS_CPP_DIR}/nmos/json_schema.cpp
    ${NMOS_CPP_DIR}/nmos/log_model.cpp
    ${NMOS_CPP_DIR}/nmos/logging_api.cpp
    ${NMOS_CPP_DIR}/nmos/mdns.cpp
    ${NMOS_CPP_DIR}/nmos/mdns_api.cpp
    ${NMOS_CPP_DIR}/nmos/node_api.cpp
    ${NMOS_CPP_DIR}/nmos/node_api_target_handler.cpp
    ${NMOS_CPP_DIR}/nmos/node_behaviour.cpp
    ${NMOS_CPP_DIR}/nmos/node_resource.cpp
    ${NMOS_CPP_DIR}/nmos/node_resources.cpp
    ${NMOS_CPP_DIR}/nmos/node_server.cpp
    ${NMOS_CPP_DIR}/nmos/process_utils.cpp
    ${NMOS_CPP_DIR}/nmos/query_api.cpp
    ${NMOS_CPP_DIR}/nmos/query_utils.cpp
    ${NMOS_CPP_DIR}/nmos/query_ws_api.cpp
    ${NMOS_CPP_DIR}/nmos/registration_api.cpp
    ${NMOS_CPP_DIR}/nmos/registry_resources.cpp
    ${NMOS_CPP_DIR}/nmos/registry_server.cpp
    ${NMOS_CPP_DIR}/nmos/resource.cpp
    ${NMOS_CPP_DIR}/nmos/resources.cpp
    ${NMOS_CPP_DIR}/nmos/sdp_utils.cpp
    ${NMOS_CPP_DIR}/nmos/server_utils.cpp
    ${NMOS_CPP_DIR}/nmos/settings.cpp
    ${NMOS_CPP_DIR}/nmos/settings_api.cpp
    ${NMOS_CPP_DIR}/nmos/system_api.cpp
    ${NMOS_CPP_DIR}/nmos/system_resources.cpp
    )
set(NMOS_CPP_NMOS_HEADERS
    ${NMOS_CPP_DIR}/nmos/activation_mode.h
    ${NMOS_CPP_DIR}/nmos/admin_ui.h
    ${NMOS_CPP_DIR}/nmos/api_downgrade.h
    ${NMOS_CPP_DIR}/nmos/api_utils.h
    ${NMOS_CPP_DIR}/nmos/api_version.h
    ${NMOS_CPP_DIR}/nmos/channels.h
    ${NMOS_CPP_DIR}/nmos/client_utils.h
    ${NMOS_CPP_DIR}/nmos/colorspace.h
    ${NMOS_CPP_DIR}/nmos/components.h
    ${NMOS_CPP_DIR}/nmos/copyable_atomic.h
    ${NMOS_CPP_DIR}/nmos/connection_activation.h
    ${NMOS_CPP_DIR}/nmos/connection_api.h
    ${NMOS_CPP_DIR}/nmos/connection_resources.h
    ${NMOS_CPP_DIR}/nmos/device_type.h
    ${NMOS_CPP_DIR}/nmos/event_type.h
    ${NMOS_CPP_DIR}/nmos/events_api.h
    ${NMOS_CPP_DIR}/nmos/events_resources.h
    ${NMOS_CPP_DIR}/nmos/events_ws_api.h
    ${NMOS_CPP_DIR}/nmos/filesystem_route.h
    ${NMOS_CPP_DIR}/nmos/format.h
    ${NMOS_CPP_DIR}/nmos/group_hint.h
    ${NMOS_CPP_DIR}/nmos/health.h
    ${NMOS_CPP_DIR}/nmos/id.h
    ${NMOS_CPP_DIR}/nmos/interlace_mode.h
    ${NMOS_CPP_DIR}/nmos/is04_versions.h
    ${NMOS_CPP_DIR}/nmos/is05_versions.h
    ${NMOS_CPP_DIR}/nmos/is07_versions.h
    ${NMOS_CPP_DIR}/nmos/is09_versions.h
    ${NMOS_CPP_DIR}/nmos/json_fields.h
    ${NMOS_CPP_DIR}/nmos/json_schema.h
    ${NMOS_CPP_DIR}/nmos/log_gate.h
    ${NMOS_CPP_DIR}/nmos/log_manip.h
    ${NMOS_CPP_DIR}/nmos/log_model.h
    ${NMOS_CPP_DIR}/nmos/logging_api.h
    ${NMOS_CPP_DIR}/nmos/mdns.h
    ${NMOS_CPP_DIR}/nmos/mdns_api.h
    ${NMOS_CPP_DIR}/nmos/media_type.h
    ${NMOS_CPP_DIR}/nmos/model.h
    ${NMOS_CPP_DIR}/nmos/mutex.h
    ${NMOS_CPP_DIR}/nmos/node_api.h
    ${NMOS_CPP_DIR}/nmos/node_api_target_handler.h
    ${NMOS_CPP_DIR}/nmos/node_behaviour.h
    ${NMOS_CPP_DIR}/nmos/node_resource.h
    ${NMOS_CPP_DIR}/nmos/node_resources.h
    ${NMOS_CPP_DIR}/nmos/node_server.h
    ${NMOS_CPP_DIR}/nmos/paging_utils.h
    ${NMOS_CPP_DIR}/nmos/process_utils.h
    ${NMOS_CPP_DIR}/nmos/query_api.h
    ${NMOS_CPP_DIR}/nmos/query_utils.h
    ${NMOS_CPP_DIR}/nmos/query_ws_api.h
    ${NMOS_CPP_DIR}/nmos/random.h
    ${NMOS_CPP_DIR}/nmos/rational.h
    ${NMOS_CPP_DIR}/nmos/registration_api.h
    ${NMOS_CPP_DIR}/nmos/registry_resources.h
    ${NMOS_CPP_DIR}/nmos/registry_server.h
    ${NMOS_CPP_DIR}/nmos/resource.h
    ${NMOS_CPP_DIR}/nmos/resources.h
    ${NMOS_CPP_DIR}/nmos/sdp_utils.h
    ${NMOS_CPP_DIR}/nmos/server.h
    ${NMOS_CPP_DIR}/nmos/server_utils.h
    ${NMOS_CPP_DIR}/nmos/settings.h
    ${NMOS_CPP_DIR}/nmos/settings_api.h
    ${NMOS_CPP_DIR}/nmos/slog.h
    ${NMOS_CPP_DIR}/nmos/ssl_context_options.h
    ${NMOS_CPP_DIR}/nmos/string_enum.h
    ${NMOS_CPP_DIR}/nmos/system_api.h
    ${NMOS_CPP_DIR}/nmos/system_resources.h
    ${NMOS_CPP_DIR}/nmos/tai.h
    ${NMOS_CPP_DIR}/nmos/thread_utils.h
    ${NMOS_CPP_DIR}/nmos/transfer_characteristic.h
    ${NMOS_CPP_DIR}/nmos/transport.h
    ${NMOS_CPP_DIR}/nmos/type.h
    ${NMOS_CPP_DIR}/nmos/version.h
    ${NMOS_CPP_DIR}/nmos/websockets.h
    )

set(NMOS_CPP_PPLX_SOURCES
    ${NMOS_CPP_DIR}/pplx/pplx_utils.cpp
    )
set(NMOS_CPP_PPLX_HEADERS
    ${NMOS_CPP_DIR}/pplx/pplx_utils.h
    )

set(NMOS_CPP_RQL_SOURCES
    ${NMOS_CPP_DIR}/rql/rql.cpp
    )
set(NMOS_CPP_RQL_HEADERS
    ${NMOS_CPP_DIR}/rql/rql.h
    )

set(NMOS_CPP_SDP_SOURCES
    ${NMOS_CPP_DIR}/sdp/sdp_grammar.cpp
    )
set(NMOS_CPP_SDP_HEADERS
    ${NMOS_CPP_DIR}/sdp/json.h
    ${NMOS_CPP_DIR}/sdp/ntp.h
    ${NMOS_CPP_DIR}/sdp/sdp.h
    ${NMOS_CPP_DIR}/sdp/sdp_grammar.h
    )

set(NMOS_CPP_SLOG_HEADERS
    ${NMOS_CPP_DIR}/slog/all_in_one.h
    )

add_library(
    nmos-cpp_static STATIC
    ${NMOS_CPP_BST_SOURCES}
    ${NMOS_CPP_BST_HEADERS}
    ${NMOS_CPP_CPPREST_SOURCES}
    ${NMOS_CPP_CPPREST_HEADERS}
    ${NMOS_CPP_NMOS_SOURCES}
    ${NMOS_CPP_NMOS_HEADERS}
    ${NMOS_CPP_PPLX_SOURCES}
    ${NMOS_CPP_PPLX_HEADERS}
    ${NMOS_CPP_RQL_SOURCES}
    ${NMOS_CPP_RQL_HEADERS}
    ${NMOS_CPP_SDP_SOURCES}
    ${NMOS_CPP_SDP_HEADERS}
    ${NMOS_CPP_SLOG_HEADERS}
    )

source_group("bst\\Source Files" FILES ${NMOS_CPP_BST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_CPPREST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_NMOS_SOURCES})
source_group("pplx\\Source Files" FILES ${NMOS_CPP_PPLX_SOURCES})
source_group("rql\\Source Files" FILES ${NMOS_CPP_RQL_SOURCES})
source_group("sdp\\Source Files" FILES ${NMOS_CPP_SDP_SOURCES})

source_group("bst\\Header Files" FILES ${NMOS_CPP_BST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_CPPREST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_NMOS_HEADERS})
source_group("pplx\\Header Files" FILES ${NMOS_CPP_PPLX_HEADERS})
source_group("rql\\Header Files" FILES ${NMOS_CPP_RQL_HEADERS})
source_group("sdp\\Header Files" FILES ${NMOS_CPP_SDP_HEADERS})
source_group("slog\\Header Files" FILES ${NMOS_CPP_SLOG_HEADERS})

target_link_libraries(
    nmos-cpp_static
    mdns_static
    cpprestsdk::cpprest
    json_schema_validator_static
    nmos_is04_schemas_static
    nmos_is05_schemas_static
    nmos_is09_schemas_static
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

install(TARGETS nmos-cpp_static DESTINATION lib)

install(FILES ${NMOS_CPP_BST_HEADERS} DESTINATION include/bst)
install(FILES ${NMOS_CPP_CPPREST_HEADERS} DESTINATION include/cpprest)
install(FILES ${NMOS_CPP_CPPREST_DETAILS_HEADERS} DESTINATION include/cpprest/details)
install(FILES ${NMOS_CPP_NMOS_HEADERS} DESTINATION include/nmos)
install(FILES ${NMOS_CPP_PPLX_HEADERS} DESTINATION include/pplx)
install(FILES ${NMOS_CPP_RQL_HEADERS} DESTINATION include/rql)
install(FILES ${NMOS_CPP_SDP_HEADERS} DESTINATION include/sdp)
install(FILES ${NMOS_CPP_SLOG_HEADERS} DESTINATION include/slog)
