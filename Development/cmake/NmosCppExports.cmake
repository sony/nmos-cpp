# see https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html

install(TARGETS ${NMOS_CPP_TARGETS}
    EXPORT nmos-cpp-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}${NMOS_CPP_INCLUDE_PREFIX}
    )

install(EXPORT nmos-cpp-targets
    FILE nmos-cpp-targets.cmake
    NAMESPACE nmos-cpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nmos-cpp
)

include(CMakePackageConfigHelpers)

configure_package_config_file(cmake/nmos-cpp-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/nmos-cpp-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nmos-cpp
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/nmos-cpp-config.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nmos-cpp)
