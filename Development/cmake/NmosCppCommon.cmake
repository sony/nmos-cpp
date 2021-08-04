set(USE_CONAN ON CACHE BOOL "Use Conan to acquire dependencies")
mark_as_advanced(FORCE USE_CONAN)

if(USE_CONAN)
    include(cmake/NmosCppConan.cmake)
endif()

include(GNUInstallDirs)

set(NMOS_CPP_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
if(NMOS_CPP_INCLUDE_PREFIX)
    string(APPEND NMOS_CPP_INSTALL_INCLUDEDIR "${NMOS_CPP_INCLUDE_PREFIX}")
endif()

set(NMOS_CPP_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(NMOS_CPP_INSTALL_BINDIR "${CMAKE_INSTALL_LIBDIR}")
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

if(BUILD_TESTS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # note: to see the output of any failed tests, set CTEST_OUTPUT_ON_FAILURE=1 in the environment
    # and also remember that CMake doesn't add dependencies to the "test" (or "RUN_TESTS") target
    # so after changing code under test, it is important to "make all" (or build "ALL_BUILD")
    enable_testing()
endif()

# location of additional CMake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    )

# guard against in-source builds and bad build-type strings
include(safeguards)

# set common C++ compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # default to -O3
    add_compile_options("$<IF:$<CONFIG:Debug>,-O0;-g3,-O3>")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        # required for std::this_thread::sleep_for in e.g. mdns/test/mdns_test.cpp
        # see https://stackoverflow.com/questions/12523122/what-is-glibcxx-use-nanosleep-all-about
        add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
    endif()
elseif(MSVC)
    # set CharacterSet to Unicode rather than MultiByte
    add_definitions(/DUNICODE /D_UNICODE)
endif()

# set most compiler warnings on
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    add_compile_options(-Wall -Wstrict-aliasing -fstrict-aliasing -Wextra -Wno-unused-parameter -pedantic -Wno-long-long)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5)
        add_compile_options(-Wno-missing-field-initializers)
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        add_compile_options(-fpermissive)
    endif()
elseif(MSVC)
    # see https://cmake.org/cmake/help/latest/policy/CMP0092.html
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
    add_compile_options("/FI${CMAKE_CURRENT_SOURCE_DIR}/detail/vc_disable_warnings.h")
endif()
