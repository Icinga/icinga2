#.rst:
# FindJaeger
# ------------
#
# This module finds the `Jaeger`__ library.
#
# Imported target
# ^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``Jaeger``
#   The jaeger library, if found
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module sets the following
#
# ``OPENTRACE_FOUND``
#   ``TRUE`` if system has Jaeger
# ``OPENTRACE_INCLUDE_DIRS``
#   The OPENTRACE include directories
# ``OPENTRACE_LIBRARIES``
#   The libraries needed to use OPENTRACE
# ``OPENTRACE_VERSION_STRING``
#   The OPENTRACE version
# ``OPENTRACE_HAS_TRACEF``
#   ``TRUE`` if the ``tracef()`` API is available in the system's LTTng-UST
# ``OPENTRACE_HAS_TRACELOG``
#   ``TRUE`` if the ``tracelog()`` API is available in the system's LTTng-UST

#=============================================================================
# Copyright 2018 Mania Abdi, Inc.
# Copyright 2018 Mania Abdi
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(OPENTRACE_INCLUDE_DIRS NAMES  opentracing/tracer.h)
find_library(OPENTRACE_LIBRARIES NAMES opentracing)

if(OPENTRACE_INCLUDE_DIRS AND OPENTRACE_LIBRARIES)
  # find tracef() and tracelog() support
  set(OPENTRACE_HAS_TRACEF 0)
  set(OPENTRACE_HAS_TRACELOG 0)

    set(OPENTRACE_VERSION_STRING
        "0.0.0")

  if(NOT TARGET OPENTRACE)
    add_library(OPENTRACE UNKNOWN IMPORTED)
    set_target_properties(OPENTRACE PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${OPENTRACE_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${OPENTRACE_LIBRARIES}")
  endif()

  # add libdl to required libraries
  set(OPENTRACE_LIBRARIES "${OPENTRACE_LIBRARIES}" "${CMAKE_DL_LIBS}")
endif()

# handle the QUIETLY and REQUIRED arguments and set LTTNGUST_FOUND to
# TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OPENTRACE FOUND_VAR OPENTRACE_FOUND
                                  REQUIRED_VARS OPENTRACE_LIBRARIES
                                                OPENTRACE_INCLUDE_DIRS
                                  VERSION_VAR OPENTRACE_VERSION_STRING)
mark_as_advanced(OPENTRACE_LIBRARIES OPENTRACE_INCLUDE_DIRS)
