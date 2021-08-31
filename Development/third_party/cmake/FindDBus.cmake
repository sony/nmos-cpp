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
FindDBus
---------

Finds the DBus library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``DBus::DBus``
  The DBus library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``DBus_FOUND``
  True if the system has the DBus library.
``DBus_INCLUDE_DIRS``
  Include directories needed to use DBus.
``DBus_LIBRARIES``
  Libraries needed to link to DBus.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``DBus_INCLUDE_DIR``
  The directory containing ``dbus/dbus.h`` etc.
``DBus_LIBRARY``
  The path to the DBus library.

#]=======================================================================]

find_path(DBus_INCLUDE_DIR
  NAMES dbus/dbus.h
  PATH_SUFFIXES dbus-1.0
  HINTS $ENV{DBus_ROOT} ${DBus_ROOT}
)

find_library(DBus_LIBRARY
  NAMES dbus-1
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DBus
  FOUND_VAR DBus_FOUND
  REQUIRED_VARS
    DBus_LIBRARY
    DBus_INCLUDE_DIR
)

if(DBus_FOUND)
  set(DBus_LIBRARIES ${DBus_LIBRARY})
  set(DBus_INCLUDE_DIRS ${DBus_INCLUDE_DIR})
endif()

if(DBus_FOUND AND NOT TARGET DBus::DBus)
  add_library(DBus::DBus UNKNOWN IMPORTED)
  set_target_properties(DBus::DBus PROPERTIES
    IMPORTED_LOCATION "${DBus_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${DBus_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  DBus_INCLUDE_DIR
  DBus_LIBRARY
)

cmake_policy(POP)
