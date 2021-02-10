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
set(CMAKE_CXX_STANDARD 11 CACHE STRING "Default value for CXX_STANDARD property of targets")
if(CMAKE_CXX_STANDARD STREQUAL "98")
    message(FATAL_ERROR "CMAKE_CXX_STANDARD must be 11 or higher; C++98 is not supported")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# note: to see the output of any failed tests, set CTEST_OUTPUT_ON_FAILURE=1 in the environment
# and also remember that CMake doesn't add dependencies to the "test" (or "RUN_TESTS") target
# so after changing code under test, it is important to "make all" (or build "ALL_BUILD")
enable_testing()

# location of additional CMake modules
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_BINARY_DIR}
    ${NMOS_CPP_DIR}/third_party/cmake
    ${NMOS_CPP_DIR}/cmake
    )

# location of <PackageName>Config.cmake files created by Conan
set(CMAKE_PREFIX_PATH
    ${CMAKE_PREFIX_PATH}
    ${CMAKE_BINARY_DIR}
    )

if(${USE_CONAN} AND CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    # use <PackageName>Config.cmake
    set(FIND_PACKAGE_USE_CONFIG CONFIG)
endif()

# guard against in-source builds and bad build-type strings
include(safeguards)

# enable or disable the LLDP support library (lldp_static)
set (BUILD_LLDP OFF CACHE BOOL "Build LLDP support library")

# find dependencies

# cpprestsdk
# note: 2.10.16 or higher is recommended (which is the first version with cpprestsdk-configVersion.cmake)
set (CPPRESTSDK_VERSION_MIN "2.10.11")
set (CPPRESTSDK_VERSION_CUR "2.10.17")
find_package(cpprestsdk REQUIRED ${FIND_PACKAGE_USE_CONFIG})
if (NOT cpprestsdk_VERSION)
    message(STATUS "Found cpprestsdk unknown version; minimum version: " ${CPPRESTSDK_VERSION_MIN})
elseif (cpprestsdk_VERSION VERSION_LESS CPPRESTSDK_VERSION_MIN)
    message(FATAL_ERROR "Found cpprestsdk version " ${cpprestsdk_VERSION} " that is lower than the minimum version: " ${CPPRESTSDK_VERSION_MIN})
elseif(cpprestsdk_VERSION VERSION_GREATER CPPRESTSDK_VERSION_CUR)
    message(STATUS "Found cpprestsdk version " ${cpprestsdk_VERSION} " that is higher than the current tested version: " ${CPPRESTSDK_VERSION_CUR})
else()
    message(STATUS "Found cpprestsdk version " ${cpprestsdk_VERSION})
endif()
if (TARGET cpprestsdk::cpprest)
    set(CPPRESTSDK_TARGET cpprestsdk::cpprest)
else()
    set(CPPRESTSDK_TARGET cpprestsdk::cpprestsdk)
endif()
message(STATUS "Using cpprestsdk target ${CPPRESTSDK_TARGET}")
if (DEFINED CPPREST_INCLUDE_DIR)
    message(STATUS "Using cpprestsdk include directory at ${CPPREST_INCLUDE_DIR}")
endif()

# websocketpp
# note: good idea to use same version as cpprestsdk was built with!
if(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    message(STATUS "Using configured websocketpp include directory at " ${WEBSOCKETPP_INCLUDE_DIR})
else()
    set (WEBSOCKETPP_VERSION_MIN "0.5.1")
    set (WEBSOCKETPP_VERSION_CUR "0.8.2")
    find_package(websocketpp REQUIRED ${FIND_PACKAGE_USE_CONFIG})
    if (NOT websocketpp_VERSION)
        message(STATUS "Found websocketpp unknown version; minimum version: " ${WEBSOCKETPP_VERSION_MIN})
    elseif (websocketpp_VERSION VERSION_LESS WEBSOCKETPP_VERSION_MIN)
        message(FATAL_ERROR "Found websocketpp version " ${websocketpp_VERSION} " that is lower than the minimum version: " ${WEBSOCKETPP_VERSION_MIN})
    elseif(websocketpp_VERSION VERSION_GREATER WEBSOCKETPP_VERSION_CUR)
        message(STATUS "Found websocketpp version " ${websocketpp_VERSION} " that is higher than the current tested version: " ${WEBSOCKETPP_VERSION_CUR})
    else()
        message(STATUS "Found websocketpp version " ${websocketpp_VERSION})
    endif()
    if (DEFINED WEBSOCKETPP_INCLUDE_DIR)
        message(STATUS "Using websocketpp include directory at ${WEBSOCKETPP_INCLUDE_DIR}")
    endif()
endif()

# boost
# note: some components are only required for one platform or other
# so find_package(Boost) is called after adding those components
list(APPEND FIND_BOOST_COMPONENTS system date_time regex)

# openssl
# note: good idea to use same version as cpprestsk was built with!
find_package(OpenSSL REQUIRED ${FIND_PACKAGE_USE_CONFIG})
if (TARGET OpenSSL::SSL)
    set(OPENSSL_TARGETS OpenSSL::Crypto OpenSSL::SSL)
else()
    set(OPENSSL_TARGETS OpenSSL::OpenSSL)
endif()
message(STATUS "Using OpenSSL target(s) ${OPENSSL_TARGETS}")
if (DEFINED OPENSSL_INCLUDE_DIR)
    message(STATUS "Using OpenSSL include directory at ${OPENSSL_INCLUDE_DIR}")
endif()

# platform-specific dependencies

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # find Bonjour or Avahi compatibility library for the mDNS support library (mdns_static)
    # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR aren't set, the headers and library are assumed to be installed in the system paths
    set (BONJOUR_LIB -ldns_sd)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # find resolver (for cpprest/host_utils.cpp)
    list(APPEND PLATFORM_LIBS -lresolv)

    # define __STDC_LIMIT_MACROS to work around C99 vs. C++11 bug in glibc 2.17
    # should be harmless with newer glibc or in other scenarios
    # see https://sourceware.org/bugzilla/show_bug.cgi?id=15366
    # and https://sourceware.org/ml/libc-alpha/2013-04/msg00598.html
    add_definitions(/D__STDC_LIMIT_MACROS)

    # add dependency required by nmos/filesystem_route.cpp
    if((CMAKE_CXX_COMPILER_ID MATCHES GNU) AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.3))
        list(APPEND PLATFORM_LIBS -lstdc++fs)
    else()
        list(APPEND FIND_BOOST_COMPONENTS filesystem)
    endif()

    if(BUILD_LLDP)
        # find libpcap for the LLDP support library (lldp_static)
        set (PCAP_LIB -lpcap)
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # find Bonjour for the mDNS support library (mdns_static)
    set (MDNS_SYSTEM_BONJOUR OFF CACHE BOOL "Use installed Bonjour SDK")
    if(MDNS_SYSTEM_BONJOUR)
        # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR are now set by default to the location used by the Bonjour SDK Installer (bonjoursdksetup.exe) 3.0.0
        set (BONJOUR_INCLUDE "$ENV{PROGRAMFILES}/Bonjour SDK/Include" CACHE PATH "Bonjour SDK include directory")
        set (BONJOUR_LIB_DIR "$ENV{PROGRAMFILES}/Bonjour SDK/Lib/x64" CACHE PATH "Bonjour SDK library directory")
        set (BONJOUR_LIB dnssd)
        # dnssd.lib is built with /MT, so exclude libcmt if we're building nmos-cpp with the dynamically-linked runtime library
        if(CMAKE_VERSION VERSION_LESS 3.15)
            foreach(Config ${CMAKE_CONFIGURATION_TYPES})
                string(TOUPPER ${Config} CONFIG)
                # default is /MD or /MDd
                if(NOT("${CMAKE_CXX_FLAGS_${CONFIG}}" MATCHES "/MT"))
                    message(STATUS "Excluding libcmt for ${Config} because CMAKE_CXX_FLAGS_${CONFIG} is: ${CMAKE_CXX_FLAGS_${CONFIG}}")
                    set (CMAKE_EXE_LINKER_FLAGS_${CONFIG} "${CMAKE_EXE_LINKER_FLAGS_${CONFIG}} /NODEFAULTLIB:libcmt")
                endif()
            endforeach()
        else()
            # default is "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
            if((NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY) OR (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "DLL"))
                message(STATUS "Excluding libcmt because CMAKE_MSVC_RUNTIME_LIBRARY is: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
                set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt")
            endif()
        endif()
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

    if(BUILD_LLDP)
        # find WinPcap for the LLDP support library (lldp_static)
        set (PCAP_INCLUDE_DIR "${NMOS_CPP_DIR}/third_party/WpdPack/Include" CACHE PATH "WinPcap include directory")
        set (PCAP_LIB_DIR "${NMOS_CPP_DIR}/third_party/WpdPack/Lib/x64" CACHE PATH "WinPcap library directory")
        set (PCAP_LIB wpcap)

        # enable 'new' WinPcap functions like pcap_open, pcap_findalldevs_ex
        add_definitions(/DHAVE_REMOTE)
    endif()
endif()

# since std::shared_mutex is not available until C++17
list(APPEND FIND_BOOST_COMPONENTS thread)
add_definitions(/DBST_SHARED_MUTEX_BOOST)

# find boost
set (BOOST_VERSION_MIN "1.54.0")
set (BOOST_VERSION_CUR "1.75.0")
# note: 1.57.0 doesn't work due to https://svn.boost.org/trac10/ticket/10754
find_package(Boost ${BOOST_VERSION_MIN} REQUIRED COMPONENTS ${FIND_BOOST_COMPONENTS} ${FIND_PACKAGE_USE_CONFIG})
# cope with historical versions of FindBoost.cmake
if (DEFINED Boost_VERSION_STRING)
    set(Boost_VERSION_COMPONENTS "${Boost_VERSION_STRING}")
elseif (DEFINED Boost_VERSION_MAJOR)
    set(Boost_VERSION_COMPONENTS "${Boost_VERSION_MAJOR}.${Boost_VERSION_MINOR}.${Boost_VERSION_PATCH}")
elseif (DEFINED Boost_MAJOR_VERSION)
    set(Boost_VERSION_COMPONENTS "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
elseif (DEFINED Boost_VERSION)
    set(Boost_VERSION_COMPONENTS "${Boost_VERSION}")
else()
    message(FATAL_ERROR "Boost_VERSION_STRING is not defined")
endif()
if (Boost_VERSION_COMPONENTS VERSION_LESS BOOST_VERSION_MIN)
    message(FATAL_ERROR "Found Boost version " ${Boost_VERSION_COMPONENTS} " that is lower than the minimum version: " ${BOOST_VERSION_MIN})
elseif(Boost_VERSION_COMPONENTS VERSION_GREATER BOOST_VERSION_CUR)
    message(STATUS "Found Boost version " ${Boost_VERSION_COMPONENTS} " that is higher than the current tested version: " ${BOOST_VERSION_CUR})
else()
    message(STATUS "Found Boost version " ${Boost_VERSION_COMPONENTS})
endif()
if (DEFINED Boost_INCLUDE_DIRS)
    message(STATUS "Using Boost include directories at ${Boost_INCLUDE_DIRS}")
endif()

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
# though these will be determined from INTERFACE_INCLUDE_DIRECTORIES for targets
# mentioned in target_link_libraries
include_directories(
    ${NMOS_CPP_DIR}
    ${NMOS_CPP_DIR}/third_party
    ${NMOS_CPP_DIR}/third_party/nlohmann
    ${CPPREST_INCLUDE_DIR}
    ${WEBSOCKETPP_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${BONJOUR_INCLUDE}
    ${PCAP_INCLUDE_DIR}
    )

# location of libraries
link_directories(
    ${Boost_LIBRARY_DIRS}
    ${BONJOUR_LIB_DIR}
    ${PCAP_LIB_DIR}
    )

# additional configuration for common dependencies

# cpprestsdk
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.10 AND Boost_VERSION_COMPONENTS VERSION_GREATER_EQUAL 1.58.0)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"${NMOS_CPP_DIR}/cpprest/details/boost_u_workaround.h\"")
endif()

# slog
add_definitions(/DSLOG_STATIC /DSLOG_LOGGING_SEVERITY=${SLOG_LOGGING_SEVERITY})
