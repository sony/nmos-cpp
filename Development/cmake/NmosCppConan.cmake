if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

set (NMOS_CPP_CONAN_BUILD_LIBS "missing" CACHE STRING "Semicolon separated list of libraries to build rather than download")
set (NMOS_CPP_CONAN_CPPREST_FORCE_ASIO OFF CACHE BOOL "Force C++ REST SDK to use ASIO on Windows")
set (NMOS_CPP_CONAN_OPTIONS "")
if (NMOS_CPP_CONAN_CPPREST_FORCE_ASIO)
    set (NMOS_CPP_CONAN_OPTIONS "${NMOS_CPP_CONAN_OPTIONS};cpprestsdk:http_client_impl=asio;cpprestsdk:http_listener_impl=asio")
endif()

if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    # e.g. Visual Studio
    conan_cmake_run(CONANFILE conanfile.txt
                    BASIC_SETUP
                    GENERATORS cmake_find_package_multi
                    KEEP_RPATHS
                    OPTIONS ${NMOS_CPP_CONAN_OPTIONS}
                    BUILD ${NMOS_CPP_CONAN_BUILD_LIBS})
else()
    conan_cmake_run(CONANFILE conanfile.txt
                    BASIC_SETUP
                    NO_OUTPUT_DIRS
                    GENERATORS cmake_find_package
                    KEEP_RPATHS
                    OPTIONS ${NMOS_CPP_CONAN_OPTIONS}
                    BUILD ${NMOS_CPP_CONAN_BUILD_LIBS})
endif()
