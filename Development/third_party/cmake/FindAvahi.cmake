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
FindAvahi
---------

Finds the Avahi library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Avahi::common``
  The avahi-common library
``Avahi::client``
  The avahi-client library
``Avahi::compat-libdns_sd``
  The avahi-compat-libdns_sd library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Avahi_FOUND``
  True if the system has the Avahi library.
``Avahi_INCLUDE_DIRS``
  Include directories needed to use Avahi.
``Avahi_LIBRARIES``
  Libraries needed to link to Avahi.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Avahi_INCLUDE_DIR``
  The directory containing ``avahi-client/client.h`` etc.
``Avahi_common_LIBRARY``
  The path to the avahi-common library.
``Avahi_client_LIBRARY``
  The path to the avahi-client library.
``Avahi_compat-libdns_sd_LIBRARY``
  The path to the avahi-compat-libdns_sd library.

#]=======================================================================]

find_path(Avahi_INCLUDE_DIR
  NAMES avahi-client/client.h
  HINTS $ENV{Avahi_ROOT} ${Avahi_ROOT}
)

find_library(Avahi_common_LIBRARY
  NAMES avahi-common
)
find_package(Threads)

find_library(Avahi_client_LIBRARY
  NAMES avahi-client
)
find_package(DBus)

find_library(Avahi_compat-libdns_sd_LIBRARY
  NAMES dns_sd
)

# should we also check that the daemon (avahi-daemon) is installed?
# it's not required to build...

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Avahi
  FOUND_VAR Avahi_FOUND
  REQUIRED_VARS
    Avahi_common_LIBRARY
    Avahi_client_LIBRARY
    Avahi_compat-libdns_sd_LIBRARY
    Avahi_INCLUDE_DIR
)

if(Avahi_FOUND)
  set(Avahi_LIBRARIES ${Avahi_common_LIBRARY} ${Avahi_client_LIBRARY} ${Avahi_compat-libdns_sd_LIBRARY})
  set(Avahi_INCLUDE_DIRS ${Avahi_INCLUDE_DIR})
endif()

if(Avahi_FOUND AND NOT TARGET Avahi::common)
  add_library(Avahi::common UNKNOWN IMPORTED)
  set_target_properties(Avahi::common PROPERTIES
    IMPORTED_LOCATION "${Avahi_common_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Avahi_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "Threads::Threads"
  )
endif()

if(Avahi_FOUND AND NOT TARGET Avahi::client)
  add_library(Avahi::client UNKNOWN IMPORTED)
  set_target_properties(Avahi::client PROPERTIES
    IMPORTED_LOCATION "${Avahi_client_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Avahi_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "Avahi::common;DBus::DBus"
  )
endif()

if(Avahi_FOUND AND NOT TARGET Avahi::compat-libdns_sd)
  add_library(Avahi::compat-libdns_sd UNKNOWN IMPORTED)
  set_target_properties(Avahi::compat-libdns_sd PROPERTIES
    IMPORTED_LOCATION "${Avahi_compat-libdns_sd_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Avahi_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "Avahi::client"
  )
endif()

mark_as_advanced(
  Avahi_INCLUDE_DIR
  Avahi_common_LIBRARY
  Avahi_client_LIBRARY
  Avahi_compat-libdns_sd_LIBRARY
)

cmake_policy(POP)
