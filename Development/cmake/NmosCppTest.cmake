# nmos-cpp-test executable

set(NMOS_CPP_TEST_SOURCES
    nmos-cpp-test/main.cpp
    )
set(NMOS_CPP_TEST_HEADERS
    )

set(NMOS_CPP_TEST_BST_TEST_SOURCES
    )
set(NMOS_CPP_TEST_BST_TEST_HEADERS
    bst/test/test.h
    )

set(NMOS_CPP_TEST_CPPREST_TEST_SOURCES
    cpprest/test/api_router_test.cpp
    cpprest/test/basic_utils_test.cpp
    cpprest/test/http_utils_test.cpp
    cpprest/test/json_utils_test.cpp
    cpprest/test/json_visit_test.cpp
    cpprest/test/regex_utils_test.cpp
    cpprest/test/ws_listener_test.cpp
    )
set(NMOS_CPP_TEST_CPPREST_TEST_HEADERS
    )

if(NMOS_CPP_BUILD_LLDP)
    set(NMOS_CPP_TEST_LLDP_TEST_SOURCES
        lldp/test/lldp_test.cpp
        )
    set(NMOS_CPP_TEST_LLDP_TEST_HEADERS
        )
endif()

set(NMOS_CPP_TEST_MDNS_TEST_SOURCES
    mdns/test/core_test.cpp
    mdns/test/mdns_test.cpp
    )
set(NMOS_CPP_TEST_MDNS_TEST_HEADERS
    )

set(NMOS_CPP_TEST_NMOS_TEST_SOURCES
    nmos/test/api_utils_test.cpp
    nmos/test/capabilities_test.cpp
    nmos/test/channels_test.cpp
	nmos/test/configuration_utils_test.cpp
    nmos/test/control_protocol_test.cpp
    nmos/test/did_sdid_test.cpp
    nmos/test/event_type_test.cpp
    nmos/test/json_validator_test.cpp
    nmos/test/jwt_validation_test.cpp
    nmos/test/paging_utils_test.cpp
    nmos/test/query_api_test.cpp
    nmos/test/sdp_test_utils.cpp
    nmos/test/sdp_utils_test.cpp
    nmos/test/slog_test.cpp
    nmos/test/system_resources_test.cpp
    nmos/test/video_jxsv_test.cpp
    )
set(NMOS_CPP_TEST_NMOS_TEST_HEADERS
    nmos/test/sdp_test_utils.h
    )

set(NMOS_CPP_TEST_PPLX_TEST_SOURCES
    pplx/test/pplx_utils_test.cpp
    )
set(NMOS_CPP_TEST_PPLX_TEST_HEADERS
    )

set(NMOS_CPP_TEST_RQL_TEST_SOURCES
    rql/test/rql_test.cpp
    )
set(NMOS_CPP_TEST_RQL_TEST_HEADERS
    )

set(NMOS_CPP_TEST_SDP_TEST_SOURCES
    sdp/test/sdp_test.cpp
    )
set(NMOS_CPP_TEST_SDP_TEST_HEADERS
    )

add_executable(
    nmos-cpp-test
    ${NMOS_CPP_TEST_SOURCES}
    ${NMOS_CPP_TEST_HEADERS}
    ${NMOS_CPP_TEST_BST_TEST_SOURCES}
    ${NMOS_CPP_TEST_BST_TEST_HEADERS}
    ${NMOS_CPP_TEST_CPPREST_TEST_SOURCES}
    ${NMOS_CPP_TEST_CPPREST_TEST_HEADERS}
    ${NMOS_CPP_TEST_LLDP_TEST_SOURCES}
    ${NMOS_CPP_TEST_LLDP_TEST_HEADERS}
    ${NMOS_CPP_TEST_MDNS_TEST_SOURCES}
    ${NMOS_CPP_TEST_MDNS_TEST_HEADERS}
    ${NMOS_CPP_TEST_NMOS_TEST_SOURCES}
    ${NMOS_CPP_TEST_NMOS_TEST_HEADERS}
    ${NMOS_CPP_TEST_PPLX_TEST_SOURCES}
    ${NMOS_CPP_TEST_PPLX_TEST_HEADERS}
    ${NMOS_CPP_TEST_RQL_TEST_SOURCES}
    ${NMOS_CPP_TEST_RQL_TEST_HEADERS}
    ${NMOS_CPP_TEST_SDP_TEST_SOURCES}
    ${NMOS_CPP_TEST_SDP_TEST_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_TEST_SOURCES})
source_group("bst\\test\\Source Files" FILES ${NMOS_CPP_TEST_BST_TEST_SOURCES})
source_group("cpprest\\test\\Source Files" FILES ${NMOS_CPP_TEST_CPPREST_TEST_SOURCES})
source_group("lldp\\test\\Source Files" FILES ${NMOS_CPP_TEST_LLDP_TEST_SOURCES})
source_group("mdns\\test\\Source Files" FILES ${NMOS_CPP_TEST_MDNS_TEST_SOURCES})
source_group("nmos\\test\\Source Files" FILES ${NMOS_CPP_TEST_NMOS_TEST_SOURCES})
source_group("pplx\\test\\Source Files" FILES ${NMOS_CPP_TEST_PPLX_TEST_SOURCES})
source_group("rql\\test\\Source Files" FILES ${NMOS_CPP_TEST_RQL_TEST_SOURCES})
source_group("sdp\\test\\Source Files" FILES ${NMOS_CPP_TEST_SDP_TEST_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_TEST_HEADERS})
source_group("bst\\test\\Header Files" FILES ${NMOS_CPP_TEST_BST_TEST_HEADERS})
source_group("cpprest\\test\\Header Files" FILES ${NMOS_CPP_TEST_CPPREST_TEST_HEADERS})
source_group("lldp\\test\\Header Files" FILES ${NMOS_CPP_TEST_LLDP_TEST_HEADERS})
source_group("mdns\\test\\Header Files" FILES ${NMOS_CPP_TEST_MDNS_TEST_HEADERS})
source_group("nmos\\test\\Header Files" FILES ${NMOS_CPP_TEST_NMOS_TEST_HEADERS})
source_group("pplx\\test\\Header Files" FILES ${NMOS_CPP_TEST_PPLX_TEST_HEADERS})
source_group("rql\\test\\Header Files" FILES ${NMOS_CPP_TEST_RQL_TEST_HEADERS})
source_group("sdp\\test\\Header Files" FILES ${NMOS_CPP_TEST_SDP_TEST_HEADERS})

target_link_libraries(
    nmos-cpp-test
    nmos-cpp::compile-settings
    nmos-cpp::nmos-cpp
    nmos-cpp::mdns
    nmos-cpp::cpprestsdk
    nmos-cpp::Boost
    nmos-cpp::jwt-cpp
    )
if(NMOS_CPP_BUILD_LLDP)
    target_link_libraries(
        nmos-cpp-test
        nmos-cpp::lldp
        )
endif()
# root directory to find e.g. bst/test/test.h
# third_party to find e.g. catch/catch.hpp
target_include_directories(nmos-cpp-test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )

include(Catch)

catch_discover_tests(nmos-cpp-test EXTRA_ARGS -r compact)
