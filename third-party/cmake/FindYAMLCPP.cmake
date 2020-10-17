#.rst:
# FindYAMLCPP
# ------------
#
# This module finds the `yaml cpp`__ library.
#
# Imported target
# ^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``Yaml``
#   The yaml library, if found
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module sets the following
#
# ``YAML_FOUND``
#   ``TRUE`` if system has yamlcpp
# ``YAML_INCLUDE_DIRS``
#   The YAML include directories
# ``YAML_LIBRARIES``
#   The libraries needed to use YAML
# ``YAML_VERSION_STRING``
#   The YAML version

#=============================================================================
# Copyright 2018 Mania Abdi.
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

find_path(YAMLCPP_INCLUDE_DIRS NAMES  yaml-cpp/yaml.h)
find_library(YAMLCPP_LIBRARIES NAMES yaml-cpp)

if(YAMLCPP_INCLUDE_DIRS AND YAMLCPP_LIBRARIES)
  # find tracef() and tracelog() support
  set(YAMLCPP_HAS_TRACEF 0)
  set(YAMLCPP_HAS_TRACELOG 0)

    set(YAMLCPP_VERSION_STRING
        "0.0.0")

  if(NOT TARGET YAMLCPP)
    add_library(YAMLCPP UNKNOWN IMPORTED)
    set_target_properties(YAMLCPP PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${YAMLCPP_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS}"
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${YAMLCPP_LIBRARIES}")
  endif()

  # add libdl to required libraries
  set(YAMLCPP_LIBRARIES ${YAMLCPP_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

# handle the QUIETLY and REQUIRED arguments and set YAMLCPP_FOUND to
# TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(YAMLCPP FOUND_VAR YAMLCPP_FOUND
                                  REQUIRED_VARS YAMLCPP_LIBRARIES
                                                YAMLCPP_INCLUDE_DIRS
                                  VERSION_VAR YAMLCPP_VERSION_STRING)
mark_as_advanced(YAMLCPP_LIBRARIES YAMLCPP_INCLUDE_DIRS)
