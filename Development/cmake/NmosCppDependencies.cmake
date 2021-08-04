# Boost

set(BOOST_VERSION_MIN "1.54.0")
set(BOOST_VERSION_CUR "1.76.0")
# note: 1.57.0 doesn't work due to https://svn.boost.org/trac10/ticket/10754
# note: some components are only required for one platform or other
# so find_package(Boost) is called after adding those components
# adding the "headers" component seems to be unnecessary (and the target alias "boost" doesn't work at all)
list(APPEND FIND_BOOST_COMPONENTS system date_time regex)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if((CMAKE_CXX_COMPILER_ID MATCHES GNU) AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.3))
    else()
        # add filesystem (for bst/filesystem.h, used by nmos/filesystem_route.cpp)
        list(APPEND FIND_BOOST_COMPONENTS filesystem)
    endif()
endif()
# since std::shared_mutex is not available until C++17
# see bst/shared_mutex.h
list(APPEND FIND_BOOST_COMPONENTS thread)
find_package(Boost ${BOOST_VERSION_MIN} REQUIRED COMPONENTS ${FIND_BOOST_COMPONENTS} ${FIND_PACKAGE_MODE})
# cope with historical versions of FindBoost.cmake
if(DEFINED Boost_VERSION_STRING)
    set(Boost_VERSION_COMPONENTS "${Boost_VERSION_STRING}")
elseif(DEFINED Boost_VERSION_MAJOR)
    set(Boost_VERSION_COMPONENTS "${Boost_VERSION_MAJOR}.${Boost_VERSION_MINOR}.${Boost_VERSION_PATCH}")
elseif(DEFINED Boost_MAJOR_VERSION)
    set(Boost_VERSION_COMPONENTS "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
elseif(DEFINED Boost_VERSION)
    set(Boost_VERSION_COMPONENTS "${Boost_VERSION}")
else()
    message(FATAL_ERROR "Boost_VERSION_STRING is not defined")
endif()
if(Boost_VERSION_COMPONENTS VERSION_LESS BOOST_VERSION_MIN)
    message(FATAL_ERROR "Found Boost version " ${Boost_VERSION_COMPONENTS} " that is lower than the minimum version: " ${BOOST_VERSION_MIN})
elseif(Boost_VERSION_COMPONENTS VERSION_GREATER BOOST_VERSION_CUR)
    message(STATUS "Found Boost version " ${Boost_VERSION_COMPONENTS} " that is higher than the current tested version: " ${BOOST_VERSION_CUR})
else()
    message(STATUS "Found Boost version " ${Boost_VERSION_COMPONENTS})
endif()
if(DEFINED Boost_INCLUDE_DIRS)
    message(STATUS "Using Boost include directories at ${Boost_INCLUDE_DIRS}")
endif()
# Boost_LIBRARIES is provided by the CMake FindBoost.cmake module and recently also by Conan for most generators
# but with cmake_find_package_multi it isn't, so map the required components to targets instead
if(NOT DEFINED Boost_LIBRARIES)
    # doesn't seem necessary to add "headers" or "boost"
    string(REGEX REPLACE "([^;]+)" "Boost::\\1" Boost_LIBRARIES "${FIND_BOOST_COMPONENTS}")
endif()
message(STATUS "Using Boost libraries ${Boost_LIBRARIES}")

add_library(Boost INTERFACE)
target_link_libraries(Boost INTERFACE "${Boost_LIBRARIES}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Boost.Uuid needs and therefore auto-links bcrypt by default on Windows since 1.67.0
    # but provides this definition to force that behaviour because if find_package(Boost)
    # foundBoostConfig.cmake, the Boost:: targets all define BOOST_ALL_NO_LIB
    target_compile_definitions(
        Boost INTERFACE
        BOOST_UUID_FORCE_AUTO_LINK
        )
    # define _WIN32_WINNT because Boost.Asio gets terribly noisy otherwise
    # note: adding a force include for <sdkddkver.h> could be another option
    # see:
    #   https://docs.microsoft.com/en-gb/cpp/porting/modifying-winver-and-win32-winnt
    #   https://stackoverflow.com/questions/9742003/platform-detection-in-cmake
    if(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 10) # Windows 10
        set(_WIN32_WINNT 0x0A00)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.3) # Windows 8.1
        set(_WIN32_WINNT 0x0603)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.2) # Windows 8
        set(_WIN32_WINNT 0x0602)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.1) # Windows 7
        set(_WIN32_WINNT 0x0601)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.0) # Windows Vista
        set(_WIN32_WINNT 0x0600)
    else() # Windows XP (5.1)
        set(_WIN32_WINNT 0x0501)
    endif()
    target_compile_definitions(
        Boost INTERFACE
        _WIN32_WINNT=${_WIN32_WINNT}
        )
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        target_compile_definitions(
            Boost INTERFACE
            BOOST_RESULT_OF_USE_DECLTYPE
            )
    endif()
endif()

list(APPEND NMOS_CPP_TARGETS Boost)
add_library(nmos-cpp::Boost ALIAS Boost)

# cpprestsdk

# note: 2.10.16 or higher is recommended (which is the first version with cpprestsdk-configVersion.cmake)
set(CPPRESTSDK_VERSION_MIN "2.10.11")
set(CPPRESTSDK_VERSION_CUR "2.10.18")
find_package(cpprestsdk REQUIRED ${FIND_PACKAGE_MODE})
if(NOT cpprestsdk_VERSION)
    message(STATUS "Found cpprestsdk unknown version; minimum version: " ${CPPRESTSDK_VERSION_MIN})
elseif(cpprestsdk_VERSION VERSION_LESS CPPRESTSDK_VERSION_MIN)
    message(FATAL_ERROR "Found cpprestsdk version " ${cpprestsdk_VERSION} " that is lower than the minimum version: " ${CPPRESTSDK_VERSION_MIN})
elseif(cpprestsdk_VERSION VERSION_GREATER CPPRESTSDK_VERSION_CUR)
    message(STATUS "Found cpprestsdk version " ${cpprestsdk_VERSION} " that is higher than the current tested version: " ${CPPRESTSDK_VERSION_CUR})
else()
    message(STATUS "Found cpprestsdk version " ${cpprestsdk_VERSION})
endif()

add_library(cpprestsdk INTERFACE)
if(TARGET cpprestsdk::cpprest)
    target_link_libraries(cpprestsdk INTERFACE cpprestsdk::cpprest)
else()
    # this was required for the Conan recipe before Conan 1.25 components (which produce the fine-grained targets) were added to its package info
    target_link_libraries(cpprestsdk INTERFACE cpprestsdk::cpprestsdk)
endif()
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.10 AND Boost_VERSION_COMPONENTS VERSION_GREATER_EQUAL 1.58.0)
    target_compile_options(cpprestsdk INTERFACE
        "$<BUILD_INTERFACE:/FI${CMAKE_CURRENT_SOURCE_DIR}/cpprest/details/boost_u_workaround.h>"
        "$<INSTALL_INTERFACE:/FIcpprest/details/boost_u_workaround.h>"
        )
    # note: the Boost::boost target has been around longer but these days is an alias for Boost::headers
    # when using either BoostConfig.cmake from installed boost or FindBoost.cmake from CMake
    target_link_libraries(cpprestsdk INTERFACE Boost::boost)
endif()

list(APPEND NMOS_CPP_TARGETS cpprestsdk)
add_library(nmos-cpp::cpprestsdk ALIAS cpprestsdk)

# websocketpp

# note: good idea to use same version as cpprestsdk was built with!
if(DEFINED WEBSOCKETPP_INCLUDE_DIR)
    message(STATUS "Using configured websocketpp include directory at " ${WEBSOCKETPP_INCLUDE_DIR})
else()
    set(WEBSOCKETPP_VERSION_MIN "0.5.1")
    set(WEBSOCKETPP_VERSION_CUR "0.8.2")
    find_package(websocketpp REQUIRED ${FIND_PACKAGE_MODE})
    if(NOT websocketpp_VERSION)
        message(STATUS "Found websocketpp unknown version; minimum version: " ${WEBSOCKETPP_VERSION_MIN})
    elseif(websocketpp_VERSION VERSION_LESS WEBSOCKETPP_VERSION_MIN)
        message(FATAL_ERROR "Found websocketpp version " ${websocketpp_VERSION} " that is lower than the minimum version: " ${WEBSOCKETPP_VERSION_MIN})
    elseif(websocketpp_VERSION VERSION_GREATER WEBSOCKETPP_VERSION_CUR)
        message(STATUS "Found websocketpp version " ${websocketpp_VERSION} " that is higher than the current tested version: " ${WEBSOCKETPP_VERSION_CUR})
    else()
        message(STATUS "Found websocketpp version " ${websocketpp_VERSION})
    endif()
    if(DEFINED WEBSOCKETPP_INCLUDE_DIR)
        message(STATUS "Using websocketpp include directory at ${WEBSOCKETPP_INCLUDE_DIR}")
    endif()
endif()

add_library(websocketpp INTERFACE)
if(TARGET websocketpp::websocketpp)
    target_link_libraries(websocketpp INTERFACE websocketpp::websocketpp)
else()
    target_include_directories(websocketpp INTERFACE "${WEBSOCKETPP_INCLUDE_DIR}")
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # define __STDC_LIMIT_MACROS to work around C99 vs. C++11 bug in glibc 2.17
    # should be harmless with newer glibc or in other scenarios
    # see https://sourceware.org/bugzilla/show_bug.cgi?id=15366
    # and https://sourceware.org/ml/libc-alpha/2013-04/msg00598.html
    target_compile_definitions(
        websocketpp INTERFACE
        __STDC_LIMIT_MACROS
        )
endif()

list(APPEND NMOS_CPP_TARGETS websocketpp)
add_library(nmos-cpp::websocketpp ALIAS websocketpp)

# OpenSSL

# note: good idea to use same version as cpprestsk was built with!
find_package(OpenSSL REQUIRED ${FIND_PACKAGE_MODE})
if(DEFINED OPENSSL_INCLUDE_DIR)
    message(STATUS "Using OpenSSL include directory at ${OPENSSL_INCLUDE_DIR}")
endif()

add_library(OpenSSL INTERFACE)
if(TARGET OpenSSL::SSL)
    target_link_libraries(OpenSSL INTERFACE OpenSSL::Crypto OpenSSL::SSL)
else()
    # this was required for the Conan recipe before Conan 1.25 components (which produce the fine-grained targets) were added to its package info
    target_link_libraries(cpprestsdk INTERFACE OpenSSL::OpenSSL)
endif()

list(APPEND NMOS_CPP_TARGETS OpenSSL)
add_library(nmos-cpp::OpenSSL ALIAS OpenSSL)

# DNS-SD library

add_library(DNSSD INTERFACE)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # find Bonjour or Avahi compatibility library for the mDNS support library (mdns)
    # note: target_include_directories and target_link_directories aren't set, the headers and library are assumed to be installed in the system paths
    target_link_libraries(DNSSD INTERFACE dns_sd)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # find Bonjour for the mDNS support library (mdns)
    set(MDNS_SYSTEM_BONJOUR OFF CACHE BOOL "Use installed Bonjour SDK")
    mark_as_advanced(FORCE MDNS_SYSTEM_BONJOUR)
    if(MDNS_SYSTEM_BONJOUR)
        # note: BONJOUR_INCLUDE and BONJOUR_LIB_DIR are now set by default to the location used by the Bonjour SDK Installer (bonjoursdksetup.exe) 3.0.0
        set(BONJOUR_INCLUDE "$ENV{PROGRAMFILES}/Bonjour SDK/Include" CACHE PATH "Bonjour SDK include directory")
        set(BONJOUR_LIB_DIR "$ENV{PROGRAMFILES}/Bonjour SDK/Lib/x64" CACHE PATH "Bonjour SDK library directory")
        target_include_directories(DNSSD INTERFACE "${BONJOUR_INCLUDE}")
        target_link_directories(DNSSD INTERFACE "${BONJOUR_LIB_DIR}")
        target_link_libraries(DNSSD INTERFACE dnssd)
        # dnssd.lib is built with /MT, so exclude libcmt if we're building nmos-cpp with the dynamically-linked runtime library
        # hmm, this needs reimplementing with target_link_options
        if(CMAKE_VERSION VERSION_LESS 3.15)
            foreach(Config ${CMAKE_CONFIGURATION_TYPES})
                string(TOUPPER ${Config} CONFIG)
                # default is /MD or /MDd
                if(NOT ("${CMAKE_CXX_FLAGS_${CONFIG}}" MATCHES "/MT"))
                    message(STATUS "Excluding libcmt for ${Config} because CMAKE_CXX_FLAGS_${CONFIG} is: ${CMAKE_CXX_FLAGS_${CONFIG}}")
                    set(CMAKE_EXE_LINKER_FLAGS_${CONFIG} "${CMAKE_EXE_LINKER_FLAGS_${CONFIG}} /NODEFAULTLIB:libcmt")
                endif()
            endforeach()
        else()
            # default is "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
            if((NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY) OR (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "DLL"))
                message(STATUS "Excluding libcmt because CMAKE_MSVC_RUNTIME_LIBRARY is: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libcmt")
            endif()
        endif()
    else()
        # hm, where best to install dns_sd.h?
        set(BONJOUR_INCLUDE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/mDNSResponder/mDNSShared>"
            "$<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}>"
            )
        set(BONJOUR_SOURCES
            third_party/mDNSResponder/mDNSWindows/DLLStub/DLLStub.cpp
            )
        set_property(
            SOURCE third_party/mDNSResponder/mDNSWindows/DLLStub/DLLStub.cpp
            PROPERTY COMPILE_DEFINITIONS
                WIN32_LEAN_AND_MEAN
            )
        set(BONJOUR_HEADERS
            third_party/mDNSResponder/mDNSWindows/DLLStub/DLLStub.h
            )
        set(BONJOUR_HEADERS_INSTALL
            third_party/mDNSResponder/mDNSShared/dns_sd.h
            )

        add_library(
            Bonjour STATIC
            ${BONJOUR_SOURCES}
            ${BONJOUR_HEADERS}
            )
        set_property(TARGET Bonjour PROPERTY OUTPUT_NAME dnssd)

        source_group("Source Files" FILES ${BONJOUR_SOURCES})
        source_group("Header Files" FILES ${BONJOUR_HEADERS})

        target_include_directories(Bonjour PUBLIC
            ${BONJOUR_INCLUDE}
            )
        target_include_directories(Bonjour PRIVATE
            third_party
            )

        install(FILES ${BONJOUR_HEADERS_INSTALL} DESTINATION "${NMOS_CPP_INSTALL_INCLUDEDIR}/.")

        list(APPEND NMOS_CPP_TARGETS Bonjour)
        add_library(nmos-cpp::Bonjour ALIAS Bonjour)

        target_link_libraries(DNSSD INTERFACE nmos-cpp::Bonjour)
    endif()
endif()

list(APPEND NMOS_CPP_TARGETS DNSSD)
add_library(nmos-cpp::DNSSD ALIAS DNSSD)

# PCAP library

if(BUILD_LLDP)
    add_library(PCAP INTERFACE)

    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        # find libpcap for the LLDP support library (lldp)
        target_link_libraries(PCAP INTERFACE pcap)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        # find WinPcap for the LLDP support library (lldp)
        set(PCAP_INCLUDE_DIR "third_party/WpdPack/Include" CACHE PATH "WinPcap include directory")
        set(PCAP_LIB_DIR "third_party/WpdPack/Lib/x64" CACHE PATH "WinPcap library directory")
        set(PCAP_LIB wpcap.lib)

        # enable 'new' WinPcap functions like pcap_open, pcap_findalldevs_ex
        target_compile_definitions(PCAP INTERFACE "$<BUILD_INTERFACE:HAVE_REMOTE>")

        get_filename_component(PCAP_INCLUDE_DIR_ABSOLUTE ${PCAP_INCLUDE_DIR} ABSOLUTE)
        get_filename_component(PCAP_LIB_DIR_ABSOLUTE ${PCAP_LIB_DIR} ABSOLUTE)

        target_include_directories(PCAP INTERFACE "$<BUILD_INTERFACE:${PCAP_INCLUDE_DIR_ABSOLUTE}>")
        target_link_directories(PCAP INTERFACE "$<BUILD_INTERFACE:${PCAP_LIB_DIR_ABSOLUTE}>")
        target_link_libraries(PCAP INTERFACE "${PCAP_LIB}")

        if (IS_ABSOLUTE ${PCAP_LIB_DIR})
            target_link_directories(PCAP INTERFACE "$<INSTALL_INTERFACE:${PCAP_LIB_DIR}>")
        else()
            install(FILES "${PCAP_LIB_DIR}/${PCAP_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
            target_link_directories(PCAP INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>")
        endif()
    endif()

    list(APPEND NMOS_CPP_TARGETS PCAP)
    add_library(nmos-cpp::PCAP ALIAS PCAP)
endif()
