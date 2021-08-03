# nmos-cpp-registry executable

set(NMOS_CPP_REGISTRY_SOURCES
    nmos-cpp-registry/main.cpp
    nmos-cpp-registry/registry_implementation.cpp
    )
set(NMOS_CPP_REGISTRY_HEADERS
    nmos-cpp-registry/registry_implementation.h
    )

add_executable(
    nmos-cpp-registry
    ${NMOS_CPP_REGISTRY_SOURCES}
    ${NMOS_CPP_REGISTRY_HEADERS}
    nmos-cpp-registry/config.json
    )

source_group("Source Files" FILES ${NMOS_CPP_REGISTRY_SOURCES})
source_group("Header Files" FILES ${NMOS_CPP_REGISTRY_HEADERS})

target_link_libraries(
    nmos-cpp-registry
    nmos-cpp::nmos-cpp
    )
# root directory to find e.g. nmos-cpp-registry/registry_implementation.h
target_include_directories(nmos-cpp-registry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

list(APPEND NMOS_CPP_TARGETS nmos-cpp-registry)
