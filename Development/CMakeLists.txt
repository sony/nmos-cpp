# CMake 3.9 is required due to cpprestsdk-config.cmake using find_dependency with COMPONENTS
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

# project name
project(nmos-cpp)

# The default nmos home directory
set(NMOS_HOME_DIR ${PROJECT_SOURCE_DIR})

# The default logging severity
set(SLOG_LOGGING_SEVERITY slog::max_verbosity)

# General setup and dependency checking
include (${NMOS_HOME_DIR}/cmake/NmosCppGeneral.cmake)

# Setup for the libraries
include (${NMOS_HOME_DIR}/cmake/NmosCppLibraries.cmake)

# nmos-cpp-node executable

set(NMOS_CPP_NODE_SOURCES
    ${NMOS_HOME_DIR}/nmos-cpp-node/main.cpp
    ${NMOS_HOME_DIR}/nmos-cpp-node/node_resources_thread.cpp
    )
set(NMOS_CPP_NODE_HEADERS
    ${NMOS_HOME_DIR}/nmos-cpp-node/main_gate.h
    ${NMOS_HOME_DIR}/nmos-cpp-node/node_resources_thread.h
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
 
# nmos-cpp-registry executable
 
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

# make the Registry test program    
include (${NMOS_HOME_DIR}/cmake/NmosCppRegTest.cmake)

