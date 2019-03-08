# nmos-cpp Common CMake setup and dependency checking

# caller can set NMOS_CPP_DIR if the project is different
if (NOT DEFINED NMOS_CPP_DIR)
    set (NMOS_CPP_DIR ${PROJECT_SOURCE_DIR})
endif()

# compile-time control of logging loquacity
# use slog::never_log_severity to strip all logging at compile-time, or slog::max_verbosity for full control at run-time
set (SLOG_LOGGING_SEVERITY slog::max_verbosity CACHE STRING "Compile-time logging level, e.g. between 40 (least verbose, only fatal messages) and -40 (most verbose)")

# enable C++11
enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# note: to see the output of any failed tests, set CTEST_OUTPUT_ON_FAILURE=1 in the environment
# and also remember that CMake doesn't add dependencies to the "test" (or "RUN_TESTS") target
# so after changing code under test, it is important to "make all" (or build "ALL_BUILD")
enable_testing()

# location of additional CMake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${NMOS_CPP_DIR}/third_party/cmake
    ${NMOS_CPP_DIR}/cmake
    )

# guard against in-source builds and bad build-type strings
include(safeguards)

# find dependencies

# cpprestsdk
# note: 2.10.9 has been tested but there's no cpprestsdk-configVersion.cmake
# and CPPREST_VERSION_MAJOR, etc. also aren't exported by cpprestsdk::cpprest
find_package(cpprestsdk REQUIRED NAMES cpprestsdk cpprest)
if(cpprestsdk_FOUND)
    message(STATUS "Found cpprestsdk")
endif()
get_target_property(CPPREST_INCLUDE_DIR cpprestsdk::cpprest INTERFACE_INCLUDE_DIRECTORIES)

# websocketpp
# note: good idea to use same version as cpprestsdk was built with!
if(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    message(STATUS "Using configured websocketpp include directory at " ${WEBSOCKETPP_INCLUDE_DIR})
else()
    find_package(websocketpp 0.5.1 REQUIRED)
    if(websocketpp_FOUND)
        message(STATUS "Found websocketpp version " ${websocketpp_VERSION})
        message(STATUS "Using websocketpp include directory at " ${WEBSOCKETPP_INCLUDE_DIR})
    endif()
endif()

# boost
# note: some components are only required for one platform or other
set(FIND_BOOST_COMPONENTS system date_time regex)

# platform-specific dependencies

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # find bonjour (for mdns_static)
    # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR aren't set, the headers and library are assumed to be installed in the system paths
    set (BONJOUR_LIB libdns_sd.so)

    # define __STDC_LIMIT_MACROS to work around C99 vs. C++11 bug in glibc 2.17
    # should be harmless with newer glibc or in other scenarios
    # see https://sourceware.org/bugzilla/show_bug.cgi?id=15366
    # and https://sourceware.org/ml/libc-alpha/2013-04/msg00598.html
    add_definitions(/D__STDC_LIMIT_MACROS)

    # add dependency required by nmos/filesystem_route.cpp
    if((CMAKE_CXX_COMPILER_ID MATCHES GNU) AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.3))
        set (PLATFORM_LIBS -lstdc++fs)
    else()
        list(APPEND FIND_BOOST_COMPONENTS filesystem)
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # find bonjour (for mdns_static)
    set (MDNS_SYSTEM_BONJOUR OFF CACHE BOOL "Use installed Bonjour SDK")
    if(MDNS_SYSTEM_BONJOUR)
        # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR are now set by default to the location used by the Bonjour SDK Installer (bonjoursdksetup.exe) 3.0.0
        set (BONJOUR_INCLUDE "$ENV{PROGRAMFILES}/Bonjour SDK/Include" CACHE PATH "Bonjour SDK include directory")
        set (BONJOUR_LIB_DIR "$ENV{PROGRAMFILES}/Bonjour SDK/Lib/x64" CACHE PATH "Bonjour SDK library directory")
        set (BONJOUR_LIB dnssd)
    else()
        # note: use the patched files rather than the system installed version
        set (BONJOUR_INCLUDE "${NMOS_CPP_DIR}/third_party/mDNSResponder/mDNSShared")
        unset (BONJOUR_LIB_DIR)
        unset (BONJOUR_LIB)
        set (BONJOUR_SOURCES
            ${NMOS_CPP_DIR}/third_party/mDNSResponder/mDNSWindows/DLLStub/DLLStub.cpp
            )
        set_property(
            SOURCE ${NMOS_CPP_DIR}/third_party/mDNSResponder/mDNSWindows/DLLStub/DLLStub.cpp
            PROPERTY COMPILE_DEFINITIONS
                WIN32_LEAN_AND_MEAN
            )
        set (BONJOUR_HEADERS
            ${NMOS_CPP_DIR}/third_party/mDNSResponder/mDNSWindows/DLLStub/DLLStub.h
            )
    endif()

    # define _WIN32_WINNT because Boost.Asio gets terribly noisy otherwise
    # notes:
    #   cpprestsdk adds /D_WIN32_WINNT=0x0600 (Windows Vista) explicitly...
    #   calculating the value from CMAKE_SYSTEM_VERSION might be better?
    #   adding a force include for <ssdkddkver.h> could be another option
    # see:
    #   https://docs.microsoft.com/en-gb/cpp/porting/modifying-winver-and-win32-winnt
    #   https://stackoverflow.com/questions/9742003/platform-detection-in-cmake
    add_definitions(/D_WIN32_WINNT=0x0600)

    # does one of our dependencies result in needing to do this?
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt")
endif()

# since std::shared_mutex is not available until C++17
list(APPEND FIND_BOOST_COMPONENTS thread)
add_definitions(/DBST_SHARED_MUTEX_BOOST)

# find boost
# note: 1.57.0 doesn't work due to https://svn.boost.org/trac10/ticket/10754
find_package(Boost 1.54.0 REQUIRED COMPONENTS ${FIND_BOOST_COMPONENTS})
set(Boost_VERSION_COMPONENTS "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

# set common C++ compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif(MSVC)
    # set CharacterSet to Unicode rather than MultiByte
    add_definitions(/DUNICODE /D_UNICODE)
endif()

# set most compiler warnings on
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wstrict-aliasing -fstrict-aliasing -Wextra -Wno-unused-parameter -pedantic -Wno-long-long")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")	
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJSON_SCHEMA_BOOST_REGEX")	
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -D_GLIBCXX_USE_NANOSLEEP -DBOOST_RESULT_OF_USE_DECLTYPE -DSLOG_DETAIL_NO_REF_QUALIFIERS -DJSON_SCHEMA_BOOST_REGEX")	
    endif()
elseif(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"${NMOS_CPP_DIR}/detail/vc_disable_warnings.h\"")
endif()

# location of header files (should be using specific target_include_directories?)
include_directories(
    ${NMOS_CPP_DIR}
    ${NMOS_CPP_DIR}/third_party
    ${CPPREST_INCLUDE_DIR} # defined above from target cpprestsdk::cpprest of find_package(cpprestsdk)
    ${WEBSOCKETPP_INCLUDE_DIR} # defined by find_package(websocketpp)
    ${Boost_INCLUDE_DIRS} # defined by find_package(Boost)
    ${BONJOUR_INCLUDE} # defined above
    ${NMOS_CPP_DIR}/third_party/nlohmann
    )

# location of libraries
link_directories(
    ${Boost_LIBRARY_DIRS}
    ${BONJOUR_LIB_DIR}
    )

# additional configuration for common dependencies

# cpprestsdk
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.10 AND Boost_VERSION_COMPONENTS VERSION_GREATER_EQUAL 1.58.0)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"${NMOS_CPP_DIR}/cpprest/details/boost_u_workaround.h\"")
endif()

# slog
add_definitions(/DSLOG_STATIC /DSLOG_LOGGING_SEVERITY=${SLOG_LOGGING_SEVERITY})
