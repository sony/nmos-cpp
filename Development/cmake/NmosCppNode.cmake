# caller can set NMOS_CPP_DIR if the project is different
if(NOT DEFINED NMOS_CPP_DIR)
    set(NMOS_CPP_DIR ${PROJECT_SOURCE_DIR})
endif()

# nmos-cpp-node executable

set(NMOS_CPP_NODE_SOURCES
    ${NMOS_CPP_DIR}/nmos-cpp-node/main.cpp
    ${NMOS_CPP_DIR}/nmos-cpp-node/node_implementation.cpp
    )
set(NMOS_CPP_NODE_HEADERS
    ${NMOS_CPP_DIR}/nmos-cpp-node/node_implementation.h
    )

add_executable(
    nmos-cpp-node
    ${NMOS_CPP_NODE_SOURCES}
    ${NMOS_CPP_NODE_HEADERS}
    ${NMOS_CPP_DIR}/nmos-cpp-node/config.json
    )

source_group("Source Files" FILES ${NMOS_CPP_NODE_SOURCES})
source_group("Header Files" FILES ${NMOS_CPP_NODE_HEADERS})

target_link_libraries(
    nmos-cpp-node
    nmos-cpp_static
    nmos-cpp::cpprestsdk
    nmos-cpp::Boost
    )

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Conan packages usually don't include PDB files so suppress the resulting warning
    set_target_properties(
        nmos-cpp-node
        PROPERTIES
        LINK_FLAGS "/ignore:4099"
        )
endif()
