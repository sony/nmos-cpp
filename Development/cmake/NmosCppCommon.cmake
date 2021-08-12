set(NMOS_CPP_USE_CONAN ON CACHE BOOL "Use Conan to acquire dependencies")
mark_as_advanced(FORCE NMOS_CPP_USE_CONAN)

if(NMOS_CPP_USE_CONAN)
    include(cmake/NmosCppConan.cmake)
endif()

include(GNUInstallDirs)

set(NMOS_CPP_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
if(NMOS_CPP_INCLUDE_PREFIX)
    string(APPEND NMOS_CPP_INSTALL_INCLUDEDIR "${NMOS_CPP_INCLUDE_PREFIX}")
endif()

set(NMOS_CPP_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(NMOS_CPP_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
if(WIN32)
    string(APPEND NMOS_CPP_INSTALL_LIBDIR "/$<IF:$<CONFIG:Debug>,Debug,Release>")
    string(APPEND NMOS_CPP_INSTALL_BINDIR "/$<IF:$<CONFIG:Debug>,Debug,Release>")
endif()

# enable C++11
enable_language(CXX)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "Default value for CXX_STANDARD property of targets")
if(CMAKE_CXX_STANDARD STREQUAL "98")
    message(FATAL_ERROR "CMAKE_CXX_STANDARD must be 11 or higher; C++98 is not supported")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NMOS_CPP_BUILD_TESTS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # note: to see the output of any failed tests, set CTEST_OUTPUT_ON_FAILURE=1 in the environment
    # and also remember that CMake doesn't add dependencies to the "test" (or "RUN_TESTS") target
    # so after changing code under test, it is important to "make all" (or build "ALL_BUILD")
    enable_testing()
endif()

# location of additional CMake modules
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    )

# guard against in-source builds and bad build-type strings
include(safeguards)

# common compiler flags and warnings
include(cmake/NmosCppCompileSettings.cmake)
