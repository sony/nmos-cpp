if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

set (NMOS_CPP_CONAN_BUILD_LIBS "missing" CACHE STRING "Semicolon separated list of libraries to build rather than download")
set (NMOS_CPP_CONAN_OPTIONS "" CACHE STRING "Semicolon separated list of Conan options")

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
