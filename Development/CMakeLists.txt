# CMake 3.9 is required due to cpprestsdk-config.cmake using find_dependency with COMPONENTS
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# project name
project(nmos-cpp)

set(NMOS_HOME_DIR ${PROJECT_SOURCE_DIR})
set(SLOG_LOGGING_SEVVERITY slog::max_verbosity)

include (${NMOS_HOME_DIR}/cmake/NmosCppGeneral.cmake)

include (${NMOS_HOME_DIR}/cmake/NmosCppLibraries.cmake)

# nmos-cpp-node

set(NMOS_CPP_NODE_SOURCES
    ${NMOS_HOME_DIR}/nmos-cpp-node/main.cpp
    ${NMOS_HOME_DIR}/nmos/node_resources_thread.cpp
    )
set(NMOS_CPP_NODE_HEADERS
    ${NMOS_HOME_DIR}/nmos-cpp-node/main_gate.h
    ${NMOS_HOME_DIR}/nmos/node_resources_thread.h
    )

add_executable(    
    nmos-cpp-node    
    ${NMOS_CPP_NODE_SOURCES}    
    ${NMOS_CPP_NODE_HEADERS}
)    
source_group("Source Files" FILES ${NMOS_CPP_NODE_SOURCES})
source_group("Header Files" FILES ${NMOS_CPP_NODE_HEADERS})

target_link_libraries(
    nmos-cpp-node
    nmos_cpp_static
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )
 
# nmos-cpp-registry
 
set(NMOS_CPP_REGISTRY_SOURCES
    ${NMOS_HOME_DIR}/nmos-cpp-registry/main.cpp
    )
set(NMOS_CPP_REGISTRY_HEADERS
    ${NMOS_HOME_DIR}/nmos-cpp-registry/main_gate.h
    )
add_executable(     
    nmos-cpp-registry     
    ${NMOS_CPP_REGISTRY_SOURCES}     
    ${NMOS_CPP_REGISTRY_HEADERS}  
) 
source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_SOURCES})   
source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_HEADERS})
 
target_link_libraries(
    nmos-cpp-registry
    nmos_cpp_static
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
    ${NMOS_HOME_DIR}/nmos-cpp-registry/test/main.cpp
    )
set(NMOS_CPP_REGISTRY_TEST_HEADERS
    )
 
add_executable(
    nmos-cpp-registry-test
    ${NMOS_CPP_REGISTRY_TEST_SOURCES} 
    ${NMOS_CPP_REGISTRY_TEST_HEADERS}
)

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_TEST_SOURCES})
source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_TEST_HEADERS})

target_link_libraries(
    nmos-cpp-registry-test
    nmos_cpp_registry_test_static
    mdns_static
    cpprestsdk::cpprest
    ${BONJOUR_LIB}
    ${PLATFORM_LIBS}
    ${Boost_LIBRARIES}
    )

include(Catch)

catch_discover_tests(nmos-cpp-registry-test EXTRA_ARGS -r compact)


