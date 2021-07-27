# CMake instructions for making the nmos-cpp test program

# caller can set NMOS_CPP_DIR if the project is different
if (NOT DEFINED NMOS_CPP_DIR)
    set (NMOS_CPP_DIR ${PROJECT_SOURCE_DIR})
endif()

# nmos-cpp-test

set(NMOS_CPP_TEST_SOURCES
    ${NMOS_CPP_DIR}/nmos-cpp-test/main.cpp
    )
set(NMOS_CPP_TEST_HEADERS
    )

set(NMOS_CPP_TEST_BST_TEST_SOURCES
    )
set(NMOS_CPP_TEST_BST_TEST_HEADERS
    ${NMOS_CPP_DIR}/bst/test/test.h
    )

set(NMOS_CPP_TEST_CPPREST_TEST_SOURCES
    ${NMOS_CPP_DIR}/cpprest/test/api_router_test.cpp
    ${NMOS_CPP_DIR}/cpprest/test/http_utils_test.cpp
    ${NMOS_CPP_DIR}/cpprest/test/json_utils_test.cpp
    ${NMOS_CPP_DIR}/cpprest/test/json_visit_test.cpp
    ${NMOS_CPP_DIR}/cpprest/test/regex_utils_test.cpp
    ${NMOS_CPP_DIR}/cpprest/test/ws_listener_test.cpp
    )
set(NMOS_CPP_TEST_CPPREST_TEST_HEADERS
    )

if (BUILD_LLDP)
    set(NMOS_CPP_TEST_LLDP_TEST_SOURCES
        ${NMOS_CPP_DIR}/lldp/test/lldp_test.cpp
        )
    set(NMOS_CPP_TEST_LLDP_TEST_HEADERS
        )
endif()

set(NMOS_CPP_TEST_MDNS_TEST_SOURCES
    ${NMOS_CPP_DIR}/mdns/test/core_test.cpp
    ${NMOS_CPP_DIR}/mdns/test/mdns_test.cpp
    )
set(NMOS_CPP_TEST_MDNS_TEST_HEADERS
    )

set(NMOS_CPP_TEST_NMOS_TEST_SOURCES
    ${NMOS_CPP_DIR}/nmos/test/api_utils_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/channels_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/did_sdid_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/event_type_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/json_validator_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/paging_utils_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/query_api_test.cpp
    ${NMOS_CPP_DIR}/nmos/test/system_resources_test.cpp
    )
set(NMOS_CPP_TEST_NMOS_TEST_HEADERS
    )

set(NMOS_CPP_TEST_SDP_TEST_SOURCES
    ${NMOS_CPP_DIR}/sdp/test/sdp_test.cpp
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
    ${NMOS_CPP_TEST_SDP_TEST_SOURCES}
    ${NMOS_CPP_TEST_SDP_TEST_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_TEST_SOURCES})
source_group("bst\\test\\Source Files" FILES ${NMOS_CPP_TEST_BST_TEST_SOURCES})
source_group("cpprest\\test\\Source Files" FILES ${NMOS_CPP_TEST_CPPREST_TEST_SOURCES})
source_group("lldp\\test\\Source Files" FILES ${NMOS_CPP_TEST_LLDP_TEST_SOURCES})
source_group("mdns\\test\\Source Files" FILES ${NMOS_CPP_TEST_MDNS_TEST_SOURCES})
source_group("nmos\\test\\Source Files" FILES ${NMOS_CPP_TEST_NMOS_TEST_SOURCES})
source_group("sdp\\test\\Source Files" FILES ${NMOS_CPP_TEST_SDP_TEST_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_TEST_HEADERS})
source_group("bst\\test\\Header Files" FILES ${NMOS_CPP_TEST_BST_TEST_HEADERS})
source_group("cpprest\\test\\Header Files" FILES ${NMOS_CPP_TEST_CPPREST_TEST_HEADERS})
source_group("lldp\\test\\Header Files" FILES ${NMOS_CPP_TEST_LLDP_TEST_HEADERS})
source_group("mdns\\test\\Header Files" FILES ${NMOS_CPP_TEST_MDNS_TEST_HEADERS})
source_group("nmos\\test\\Header Files" FILES ${NMOS_CPP_TEST_NMOS_TEST_HEADERS})
source_group("sdp\\test\\Header Files" FILES ${NMOS_CPP_TEST_SDP_TEST_HEADERS})

target_link_libraries(
    nmos-cpp-test
    nmos-cpp_static
    mdns_static
    ${CPPRESTSDK_TARGET}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )
if (BUILD_LLDP)
    target_link_libraries(
        nmos-cpp-test
        lldp_static
        )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Conan packages usually don't include PDB files so suppress the resulting warning
    set_target_properties(
        nmos-cpp-test
        PROPERTIES
        LINK_FLAGS "/ignore:4099"
        )
endif()

include(Catch)

catch_discover_tests(nmos-cpp-test EXTRA_ARGS -r compact)
