# Cmake instructions for making the registry test program

# caller can set NMOS_HOME_DIR if the project is differend
if (NOT DEFINED NMOS_HOME_DIR)
    set (NMOS_HOME_DIR ${PROJECT_SOURCE_DIR})
endif()


# nmos-cpp-registry-test

set(NMOS_CPP_REGISTRY_TEST_SOURCES
    ${NMOS_HOME_DIR}/nmos-cpp-registry/test/main.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_HEADERS
    )

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
    ${NMOS_CPP_REGISTRY_TEST_SDP_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_SDP_HEADERS}
    ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_SOURCES}
    ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_HEADERS}
    )

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SOURCES})
source_group("bst\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_BST_TEST_SOURCES})
source_group("cpprest\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_SOURCES})
source_group("cpprest\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_SOURCES})
source_group("mdns\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_SOURCES})
source_group("nmos\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_SOURCES})
source_group("nmos\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_SOURCES})
source_group("sdp\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_SOURCES})
source_group("sdp\\test\\Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_SOURCES})

source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_HEADERS})
source_group("bst\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_BST_TEST_HEADERS})
source_group("cpprest\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_HEADERS})
source_group("cpprest\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_CPPREST_TEST_HEADERS})
source_group("mdns\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_MDNS_TEST_HEADERS})
source_group("nmos\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_HEADERS})
source_group("nmos\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_NMOS_TEST_HEADERS})
source_group("sdp\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_HEADERS})
source_group("sdp\\test\\Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_SDP_TEST_HEADERS})

target_link_libraries(
    nmos-cpp-registry-test
    nmos_cpp_static
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )



include(Catch)

catch_discover_tests(nmos-cpp-registry-test EXTRA_ARGS -r compact)
