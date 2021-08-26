#=============================================================================
# Copyright (c) 2021, NVIDIA CORPORATION.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================
cmake_policy(PUSH)
cmake_policy(VERSION 3.17)

#[=======================================================================[.rst:
FindDNSSD
---------

Finds the DNSSD library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``DNSSD::DNSSD``
  The DNSSD library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``DNSSD_FOUND``
  True if the system has the DNSSD library.
``DNSSD_VERSION``
  The version of the DNSSD library which was found.
``DNSSD_INCLUDE_DIRS``
  Include directories needed to use DNSSD.
``DNSSD_LIBRARIES``
  Libraries needed to link to DNSSD.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``DNSSD_INCLUDE_DIR``
  The directory containing ``dns_sd.h``.
``DNSSD_LIBRARY``
  The path to the DNSSD library.

#]=======================================================================]

if(WIN32)
  set(_DNSSD_PATHS "$ENV{ProgramFiles}/Bonjour SDK")
  set(_DNSSD_INCLUDE_SUFFIX Include)
  if (CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    set(_DNSSD_LIB_SUFFIX Lib/Win32)
  else()
    set(_DNSSD_LIB_SUFFIX Lib/x64)
  endif()
  set(_DNSSD_LIB dnssd)
else()
  list(APPEND _DNSSD_PATHS /usr /usr/local)
  set(_DNSSD_INCLUDE_SUFFIX include)
  set(_DNSSD_LIB_SUFFIX lib)
  set(_DNSSD_LIB dns_sd)
endif()

find_path(DNSSD_INCLUDE_DIR
  NAMES dns_sd.h
  PATHS ${_DNSSD_PATHS}
  PATH_SUFFIXES ${_DNSSD_INCLUDE_SUFFIX}
  HINTS $ENV{DNSSD_ROOT} ${DNSSD_ROOT}
)

find_library(DNSSD_LIBRARY
  NAMES ${_DNSSD_LIB}
  PATHS ${_DNSSD_PATHS}
  PATH_SUFFIXES ${_DNSSD_LIB_SUFFIX}
)

# should we also check on Linux that the daemon (mdnsd) is installed
# or on Windows that the Bonjour service and client DLL (dnssd.dll) are installed?
# they're not required to build...

if(DNSSD_INCLUDE_DIR)
  file(STRINGS "${DNSSD_INCLUDE_DIR}/dns_sd.h" _define_DNS_SD_H REGEX "^#define[ \t]+_DNS_SD_H [0-9]+$")
  if (_define_DNS_SD_H)
    # unfortunately, Apple broke their own 'rule' and have released several versions now with minor version numbers > 100
    # e.g. 878.200.35 and 1310.140.1, which cannot be reconstructed from the _DNS_SD_H value :-(
    string(REGEX REPLACE "^#define[ \t]+_DNS_SD_H ([0-9]+)$" "\\1" _DNS_SD_H "${_define_DNS_SD_H}")
    math(EXPR DNSSD_VERSION_MAJOR "${_DNS_SD_H} / 10000")
    math(EXPR DNSSD_VERSION_MINOR "${_DNS_SD_H} % 10000 / 100")
    math(EXPR DNSSD_VERSION_PATCH "${_DNS_SD_H} % 100")
    set(DNSSD_VERSION "${DNSSD_VERSION_MAJOR}.${DNSSD_VERSION_MINOR}.${DNSSD_VERSION_PATCH}")
    unset(_DNS_SD_H)
  endif()
  unset(_define_DNS_SD_H)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DNSSD
  FOUND_VAR DNSSD_FOUND
  REQUIRED_VARS
    DNSSD_LIBRARY
    DNSSD_INCLUDE_DIR
  VERSION_VAR DNSSD_VERSION
)

if(DNSSD_FOUND)
  set(DNSSD_LIBRARIES ${DNSSD_LIBRARY})
  set(DNSSD_INCLUDE_DIRS ${DNSSD_INCLUDE_DIR})
endif()

if(DNSSD_FOUND AND NOT TARGET DNSSD::DNSSD)
  add_library(DNSSD::DNSSD UNKNOWN IMPORTED)
  set_target_properties(DNSSD::DNSSD PROPERTIES
    IMPORTED_LOCATION "${DNSSD_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${DNSSD_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  DNSSD_INCLUDE_DIR
  DNSSD_LIBRARY
)

unset(_DNSSD_PATHS)
unset(_DNSSD_INCLUDE_SUFFIX)
unset(_DNSSD_LIB_SUFFIX)
unset(_DNSSD_LIB)

cmake_policy(POP)
