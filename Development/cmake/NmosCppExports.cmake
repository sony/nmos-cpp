# see https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html

install(TARGETS ${NMOS_CPP_TARGETS}
    EXPORT nmos-cpp-targets
    LIBRARY DESTINATION ${NMOS_CPP_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${NMOS_CPP_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${NMOS_CPP_INSTALL_BINDIR}
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

# export custom find-modules
# see https://discourse.cmake.org/t/exporting-packages-with-a-custom-find-module/3820

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake/FindAvahi.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake/FindDBus.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake/FindDNSSD.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nmos-cpp
    )
