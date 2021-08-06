add_library(compile-settings INTERFACE)
target_compile_features(compile-settings INTERFACE cxx_std_11)

# set common C++ compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # default to -O3
    target_compile_options(compile-settings INTERFACE "$<IF:$<CONFIG:Debug>,-O0;-g3,-O3>")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        # required for std::this_thread::sleep_for in e.g. mdns/test/mdns_test.cpp
        # see https://stackoverflow.com/questions/12523122/what-is-glibcxx-use-nanosleep-all-about
        target_compile_definitions(compile-settings INTERFACE _GLIBCXX_USE_NANOSLEEP)
    endif()
elseif(MSVC)
    # set CharacterSet to Unicode rather than MultiByte
    target_compile_definitions(compile-settings INTERFACE UNICODE _UNICODE)
endif()

# set most compiler warnings on
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_options(compile-settings INTERFACE
        -Wall
        -Wstrict-aliasing
        -fstrict-aliasing
        -Wextra
        -Wno-unused-parameter
        -pedantic
        -Wno-long-long
        )
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5)
        target_compile_options(compile-settings INTERFACE
            -Wno-missing-field-initializers
            )
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        target_compile_options(compile-settings INTERFACE
            -fpermissive
            )
    endif()
elseif(MSVC)
    # see https://cmake.org/cmake/help/latest/policy/CMP0092.html
    target_compile_options(compile-settings INTERFACE /W4)
    target_compile_options(compile-settings INTERFACE "$<BUILD_INTERFACE:/FI${CMAKE_CURRENT_SOURCE_DIR}/detail/vc_disable_warnings.h>")
    get_filename_component(NMOS_CPP_INSTALL_INCLUDEDIR_ABSOLUTE "${NMOS_CPP_INSTALL_INCLUDEDIR}" ABSOLUTE BASE_DIR "${CMAKE_INSTALL_PREFIX}")
    target_compile_options(compile-settings INTERFACE "$<INSTALL_INTERFACE:/FI${NMOS_CPP_INSTALL_INCLUDEDIR_ABSOLUTE}/detail/vc_disable_warnings.h>")

    set(COMPILE_SETTINGS_DETAIL_HEADERS
        detail/vc_disable_dll_warnings.h
        detail/vc_disable_warnings.h
        )
    install(FILES ${COMPILE_SETTINGS_DETAIL_HEADERS} DESTINATION ${NMOS_CPP_INSTALL_INCLUDEDIR}/detail)
endif()

list(APPEND NMOS_CPP_TARGETS compile-settings)
add_library(nmos-cpp::compile-settings ALIAS compile-settings)
