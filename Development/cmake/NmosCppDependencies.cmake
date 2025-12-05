# Boost

set(BOOST_VERSION_MIN "1.54.0")
set(BOOST_VERSION_CUR "1.83.0")
# note: 1.57.0 doesn't work due to https://svn.boost.org/trac10/ticket/10754
# note: some components are only required for one platform or other
# so find_package(Boost) is called after adding those components
# adding the "headers" component seems to be unnecessary (and the target alias "boost" doesn't work at all)
list(APPEND FIND_BOOST_COMPONENTS system date_time regex chrono)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.3))
        # add filesystem (for bst/filesystem.h, used by nmos/filesystem_route.cpp)
        list(APPEND FIND_BOOST_COMPONENTS filesystem)
    endif()
endif()
# since std::shared_mutex is not available until C++17
# see bst/shared_mutex.h
list(APPEND FIND_BOOST_COMPONENTS thread)
find_package(Boost ${BOOST_VERSION_MIN} REQUIRED COMPONENTS ${FIND_BOOST_COMPONENTS})
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
# but with cmake_find_package_multi it isn't, and mapping the required components to targets seems robust anyway
string(REGEX REPLACE "([^;]+)" "Boost::\\1" BOOST_TARGETS "${FIND_BOOST_COMPONENTS}")
message(STATUS "Using Boost targets ${BOOST_TARGETS}, not Boost libraries ${Boost_LIBRARIES}")

# this target means the nmos-cpp libraries can just link a single Boost dependency
# and also provides a common location to inject some additional compile definitions
add_library(Boost INTERFACE)
target_link_libraries(Boost INTERFACE "${BOOST_TARGETS}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Boost.Uuid needs and therefore auto-links bcrypt by default on Windows since 1.67.0
    # but provides this definition to force that behaviour because if find_package(Boost)
    # found BoostConfig.cmake, the Boost:: targets all define BOOST_ALL_NO_LIB
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
        set(WIN32_WINNT 0x0A00)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.3) # Windows 8.1
        set(WIN32_WINNT 0x0603)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.2) # Windows 8
        set(WIN32_WINNT 0x0602)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.1) # Windows 7
        set(WIN32_WINNT 0x0601)
    elseif(${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 6.0) # Windows Vista
        set(WIN32_WINNT 0x0600)
    else() # Windows XP (5.1)
        set(WIN32_WINNT 0x0501)
    endif()
    target_compile_definitions(
        Boost INTERFACE
        _WIN32_WINNT=${WIN32_WINNT}
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
set(CPPRESTSDK_VERSION_CUR "2.10.19")
find_package(cpprestsdk REQUIRED)
if(NOT cpprestsdk_VERSION)
    message(STATUS "Found cpprestsdk unknown version; minimum version: " ${CPPRESTSDK_VERSION_MIN})
elseif(cpprestsdk_VERSION VERSION_LESS CPPRESTSDK_VERSION_MIN)
    message(FATAL_ERROR "Found cpprestsdk version " ${cpprestsdk_VERSION} " that is lower than the minimum version: " ${CPPRESTSDK_VERSION_MIN})
elseif(cpprestsdk_VERSION VERSION_GREATER CPPRESTSDK_VERSION_CUR)
    message(STATUS "Found cpprestsdk version " ${cpprestsdk_VERSION} " that is higher than the current tested version: " ${CPPRESTSDK_VERSION_CUR})
else()
    message(STATUS "Found cpprestsdk version " ${cpprestsdk_VERSION})
endif()

# this target provides a common location to inject additional compile options
add_library(cpprestsdk INTERFACE)
target_link_libraries(cpprestsdk INTERFACE cpprestsdk::cpprest)
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.10 AND Boost_VERSION_COMPONENTS VERSION_GREATER_EQUAL 1.58.0)
    target_compile_options(cpprestsdk INTERFACE "$<BUILD_INTERFACE:/FI${CMAKE_CURRENT_SOURCE_DIR}/cpprest/details/boost_u_workaround.h>")
    target_compile_options(cpprestsdk INTERFACE "$<INSTALL_INTERFACE:/FI$<INSTALL_PREFIX>/${NMOS_CPP_INSTALL_INCLUDEDIR}/cpprest/details/boost_u_workaround.h>")
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
    find_package(websocketpp REQUIRED)
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

# this target provides a common location to inject additional compile definitions
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

# note: good idea to use same version as cpprestsdk was built with!
find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_VERSION)
    message(STATUS "Found OpenSSL unknown version")
else()
    message(STATUS "Found OpenSSL version " ${OpenSSL_VERSION})
endif()
if(DEFINED OPENSSL_INCLUDE_DIR)
    message(STATUS "Using OpenSSL include directory at ${OPENSSL_INCLUDE_DIR}")
endif()

# this target means the nmos-cpp libraries can just link a single OpenSSL dependency
add_library(OpenSSL INTERFACE)
target_link_libraries(OpenSSL INTERFACE OpenSSL::Crypto OpenSSL::SSL)

list(APPEND NMOS_CPP_TARGETS OpenSSL)
add_library(nmos-cpp::OpenSSL ALIAS OpenSSL)

# json schema validator library

set(NMOS_CPP_USE_SUPPLIED_JSON_SCHEMA_VALIDATOR OFF CACHE BOOL "Use supplied third_party/nlohmann")
if(NOT NMOS_CPP_USE_SUPPLIED_JSON_SCHEMA_VALIDATOR)
    set(JSON_SCHEMA_VALIDATOR_VERSION_MIN "2.1.0")
    set(JSON_SCHEMA_VALIDATOR_VERSION_CUR "2.3.0")
    find_package(nlohmann_json_schema_validator REQUIRED)
    if(NOT nlohmann_json_schema_validator_VERSION)
        message(STATUS "Found nlohmann_json_schema_validator unknown version; minimum version: " ${JSON_SCHEMA_VALIDATOR_VERSION_MIN})
    elseif(nlohmann_json_schema_validator_VERSION VERSION_LESS JSON_SCHEMA_VALIDATOR_VERSION_MIN)
        message(FATAL_ERROR "Found nlohmann_json_schema_validator version " ${nlohmann_json_schema_validator_VERSION} " that is lower than the minimum version: " ${JSON_SCHEMA_VALIDATOR_VERSION_MIN})
    elseif(nlohmann_json_schema_validator_VERSION VERSION_GREATER JSON_SCHEMA_VALIDATOR_VERSION_CUR)
        message(STATUS "Found nlohmann_json_schema_validator version " ${nlohmann_json_schema_validator_VERSION} " that is higher than the current tested version: " ${JSON_SCHEMA_VALIDATOR_VERSION_CUR})
    else()
        message(STATUS "Found nlohmann_json_schema_validator version " ${nlohmann_json_schema_validator_VERSION})
    endif()

    set(NLOHMANN_JSON_VERSION_MIN "3.6.0")
    set(NLOHMANN_JSON_VERSION_CUR "3.11.3")
    find_package(nlohmann_json REQUIRED)
    if(NOT nlohmann_json_VERSION)
        message(STATUS "Found nlohmann_json unknown version; minimum version: " ${NLOHMANN_JSON_VERSION_MIN})
    elseif(nlohmann_json_VERSION VERSION_LESS NLOHMANN_JSON_VERSION_MIN)
        message(FATAL_ERROR "Found nlohmann_json version " ${nlohmann_json_VERSION} " that is lower than the minimum version: " ${NLOHMANN_JSON_VERSION_MIN})
    elseif(nlohmann_json_VERSION VERSION_GREATER NLOHMANN_JSON_VERSION_CUR)
        message(STATUS "Found nlohmann_json version " ${nlohmann_json_VERSION} " that is higher than the current tested version: " ${NLOHMANN_JSON_VERSION_CUR})
    else()
        message(STATUS "Found nlohmann_json version " ${nlohmann_json_VERSION})
    endif()

    add_library(json_schema_validator INTERFACE)
    target_link_libraries(json_schema_validator INTERFACE nlohmann_json_schema_validator nlohmann_json::nlohmann_json)
else()
    message(STATUS "Using sources at third_party/nlohmann instead of external \"nlohman_json_schema_validator\" and \"nlohmann_json\" packages.")

    set(JSON_SCHEMA_VALIDATOR_SOURCES
        third_party/nlohmann/json-patch.cpp
        third_party/nlohmann/json-schema-draft7.json.cpp
        third_party/nlohmann/json-uri.cpp
        third_party/nlohmann/json-validator.cpp
        third_party/nlohmann/smtp-address-validator.cpp
        third_party/nlohmann/string-format-check.cpp
        )

    set(JSON_SCHEMA_VALIDATOR_HEADERS
        third_party/nlohmann/json-patch.hpp
        third_party/nlohmann/json-schema.hpp
        third_party/nlohmann/json.hpp
        third_party/nlohmann/smtp-address-validator.hpp
        )

    add_library(
        json_schema_validator STATIC
        ${JSON_SCHEMA_VALIDATOR_SOURCES}
        ${JSON_SCHEMA_VALIDATOR_HEADERS}
        )

    source_group("Source Files" FILES ${JSON_SCHEMA_VALIDATOR_SOURCES})
    source_group("Header Files" FILES ${JSON_SCHEMA_VALIDATOR_HEADERS})

    if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
            target_compile_definitions(
                json_schema_validator PRIVATE
                JSON_SCHEMA_BOOST_REGEX
                )
            target_link_libraries(
                json_schema_validator PRIVATE
                Boost::regex
                )
        endif()
    endif()

    target_link_libraries(
        json_schema_validator PRIVATE
        nmos-cpp::compile-settings
        )
    target_include_directories(json_schema_validator PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
        $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}/third_party>
        )
endif()

list(APPEND NMOS_CPP_TARGETS json_schema_validator)
add_library(nmos-cpp::json_schema_validator ALIAS json_schema_validator)

# DNS-SD library

# this target means the nmos-cpp libraries can link the same dependency
# even if the DLL stub library is built from the patched sources on Windows
add_library(DNSSD INTERFACE)

macro(find_avahi)
    # third_party/cmake/FindAvahi.cmake uses the package name and target namespace 'Avahi'
    # but newer revisions of the 'avahi' recipe on Conan Center Index do not override the conan default
    # for cmake package name and target namespace, which is the conan package name lower-cased...
    # find_package treats <PackageName> as case-insensitive when searching for a find module or
    # config package, and ultimately creates <PackageName>_FOUND and <PackageName>_VERSION with the
    # specified case, but that doesn't apply to targets created by the find module or config package
    # so one find_package call is sufficient here, but need to detect the different target namespace
    find_package(Avahi REQUIRED)
    if(NOT Avahi_VERSION)
        message(STATUS "Found Avahi unknown version")
    else()
        message(STATUS "Found Avahi version " ${Avahi_VERSION})
    endif()

    if(TARGET Avahi::compat-libdns_sd)
        target_link_libraries(DNSSD INTERFACE Avahi::compat-libdns_sd)
    else()
        target_link_libraries(DNSSD INTERFACE avahi::compat-libdns_sd)
    endif()
endmacro()

macro(find_mdnsresponder)
    # third_party/cmake/FindDNSSD.cmake uses the package name and target namespace 'DNSSD'
    # but newer revisions of the 'mdnsresponder' recipe on CCI may not override the conan default
    # for cmake package name and target namespace, which is the conan package name lower-cased...
    # so may need two find_package attempts here
    find_package(mdnsresponder QUIET)
    if(mdnsresponder_FOUND)
        if(NOT mdnsresponder_VERSION)
            message(STATUS "Found mdnsresponder unknown version")
        else()
            message(STATUS "Found mdnsresponder version " ${mdnsresponder_VERSION})
        endif()

        target_link_libraries(DNSSD INTERFACE mdnsresponder::mdnsresponder)
    else()
        message(STATUS "Could not find a package configuration file provided by \"mdnsresponder\".\n"
                       "Trying \"DNSSD\" instead.")
        find_package(DNSSD REQUIRED)

        if(NOT DNSSD_VERSION)
            message(STATUS "Found DNSSD unknown version")
        else()
            message(STATUS "Found DNSSD version " ${DNSSD_VERSION})
        endif()

        target_link_libraries(DNSSD INTERFACE DNSSD::DNSSD)
    endif()
endmacro()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # find Bonjour or Avahi compatibility library for the mDNS support library (mdns)
    set(NMOS_CPP_USE_AVAHI ON CACHE BOOL "Use Avahi compatibility library rather than mDNSResponder")
    if(NMOS_CPP_USE_AVAHI)
        find_avahi()
    else()
        find_mdnsresponder()
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # find Bonjour for the mDNS support library (mdns)
    # on Windows, there are three components involved - the Bonjour service, the client DLL (dnssd.dll), and the DLL stub library (dnssd.lib)
    # the first two are installed by Bonjour64.msi, which is part of the Bonjour SDK or can be extracted from BonjourPSSetup.exe (print service installer)
    # the DLL is commonly installed in C:\Windows\System32
    # the DLL stub library is provided by Bonjour SDK instead of a straightforward import library, and uses LoadLibrary and GetProcAddress to load the DLL
    # however, the DLL stub library is built with /MT and has a bug which affects DNSServiceRegisterRecord, hence we default to building a patched version
    # either way, the Bonjour service and the client DLL (dnssd.dll) still need to be installed on the target system
    set(NMOS_CPP_USE_BONJOUR_SDK OFF CACHE BOOL "Use dnssd.lib from the installed Bonjour SDK")
    mark_as_advanced(FORCE NMOS_CPP_USE_BONJOUR_SDK)
    if(NMOS_CPP_USE_BONJOUR_SDK)
        find_mdnsresponder()

        # dnssd.lib is built with /MT, so exclude libcmt if we're building nmos-cpp with the dynamically-linked runtime library
        # default is "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        # see https://cmake.org/cmake/help/latest/policy/CMP0091.html
        if((NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY) OR (${CMAKE_MSVC_RUNTIME_LIBRARY} MATCHES "DLL"))
            message(STATUS "Excluding libcmt because CMAKE_MSVC_RUNTIME_LIBRARY is: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
            target_link_options(DNSSD INTERFACE /NODEFAULTLIB:libcmt)
        endif()
    else()
        message(STATUS "Using sources at third_party/mDNSResponder instead of external \"mdnsresponder\" package.")

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

        target_link_libraries(
            Bonjour PRIVATE
            nmos-cpp::compile-settings
            )
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

if(NMOS_CPP_BUILD_LLDP)
    # this target means the nmos-cpp libraries can link the same dependency
    # whether it's based on libpcap or winpcap
    add_library(PCAP INTERFACE)

    # hmm, this needs replacing with a proper find-module
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        # find libpcap for the LLDP support library (lldp)
        message(STATUS "Using system libpcap")
        target_link_libraries(PCAP INTERFACE pcap)
    elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # find WinPcap for the LLDP support library (lldp)
        set(PCAP_INCLUDE_DIR "third_party/WpdPack/Include" CACHE PATH "WinPcap include directory")
        set(PCAP_LIB_DIR "third_party/WpdPack/Lib/x64" CACHE PATH "WinPcap library directory")
        set(PCAP_LIB wpcap.lib)
        message(STATUS "Using configured WinPcap include directory at " ${PCAP_INCLUDE_DIR})

        # enable 'new' WinPcap functions like pcap_open, pcap_findalldevs_ex
        target_compile_definitions(PCAP INTERFACE HAVE_REMOTE)

        get_filename_component(PCAP_INCLUDE_DIR_ABSOLUTE "${PCAP_INCLUDE_DIR}" ABSOLUTE)
        target_include_directories(PCAP INTERFACE "$<BUILD_INTERFACE:${PCAP_INCLUDE_DIR_ABSOLUTE}>")
        if(IS_ABSOLUTE ${PCAP_INCLUDE_DIR})
            target_include_directories(PCAP INTERFACE "$<INSTALL_INTERFACE:${PCAP_INCLUDE_DIR}>")
        else()
            # hmm, for now, not installing the headers so nothing for the INSTALL_INTERFACE
        endif()

        # using absolute paths to libraries seems more robust in the long term than separately specifying target_link_directories
        get_filename_component(PCAP_LIB_DIR_ABSOLUTE "${PCAP_LIB_DIR}" ABSOLUTE)
        get_filename_component(PCAP_LIB_ABSOLUTE "${PCAP_LIB}" ABSOLUTE BASE_DIR "${PCAP_LIB_DIR_ABSOLUTE}")
        target_link_libraries(PCAP INTERFACE "$<BUILD_INTERFACE:${PCAP_LIB_ABSOLUTE}>")
        if(IS_ABSOLUTE ${PCAP_LIB_DIR})
            target_link_libraries(PCAP INTERFACE "$<INSTALL_INTERFACE:${PCAP_LIB_ABSOLUTE}>")
        else()
            install(FILES "${PCAP_LIB_DIR}/${PCAP_LIB}" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
            target_link_libraries(PCAP INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR}/${PCAP_LIB}>")
        endif()
    endif()

    list(APPEND NMOS_CPP_TARGETS PCAP)
    add_library(nmos-cpp::PCAP ALIAS PCAP)
endif()

# jwt library

set(NMOS_CPP_USE_SUPPLIED_JWT_CPP OFF CACHE BOOL "Use supplied third_party/jwt-cpp")
if(NOT NMOS_CPP_USE_SUPPLIED_JWT_CPP)
    set(JWT_VERSION_MIN "0.5.1")
    set(JWT_VERSION_CUR "0.7.0")
    find_package(jwt-cpp REQUIRED)
    if(NOT jwt-cpp_VERSION)
        message(STATUS "Found jwt-cpp unknown version; minimum version: " ${JWT_VERSION_MIN})
    elseif(jwt-cpp_VERSION VERSION_LESS JWT_VERSION_MIN)
        message(FATAL_ERROR "Found jwt-cpp version " ${jwt-cpp_VERSION} " that is lower than the minimum version: " ${JWT_VERSION_MIN})
    elseif(jwt-cpp_VERSION VERSION_GREATER JWT_VERSION_CUR)
        message(STATUS "Found jwt-cpp version " ${jwt-cpp_VERSION} " that is higher than the current tested version: " ${JWT_VERSION_CUR})
    else()
        message(STATUS "Found jwt-cpp version " ${jwt-cpp_VERSION})
    endif()

    add_library(jwt-cpp INTERFACE)
    target_link_libraries(jwt-cpp INTERFACE jwt-cpp::jwt-cpp)
else()
    message(STATUS "Using sources at third_party/jwt-cpp instead of external \"jwt-cpp\" package.")
    
    add_library(jwt-cpp INTERFACE)
	target_include_directories(jwt-cpp INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
        $<INSTALL_INTERFACE:${NMOS_CPP_INSTALL_INCLUDEDIR}/third_party>
        )
endif()

target_compile_definitions(
    jwt-cpp INTERFACE
    JWT_DISABLE_PICOJSON
    )

set_target_properties(jwt-cpp PROPERTIES LINKER_LANGUAGE CXX)
list(APPEND NMOS_CPP_TARGETS jwt-cpp)
add_library(nmos-cpp::jwt-cpp ALIAS jwt-cpp)
