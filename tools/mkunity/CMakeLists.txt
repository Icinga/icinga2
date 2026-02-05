# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

define_property(
  SOURCE
  PROPERTY EXCLUDE_UNITY_BUILD
  BRIEF_DOCS "Whether to exclude the source file from unity builds"
  FULL_DOCS "Specified whether a source file should be excluded from unity builds and should be built separately"
)

if(ICINGA2_UNITY_BUILD)
  set(mkunity_SOURCES
    mkunity.c
  )

  add_executable(mkunity ${mkunity_SOURCES})

  set_target_properties (
    mkunity PROPERTIES
    FOLDER Bin
  )

  function(MKUNITY_TARGET Target Prefix UnityInputRef)
    set(UnityInput ${${UnityInputRef}})
    set(UnityOutput ${CMAKE_CURRENT_BINARY_DIR}/${Target}_unity.cpp)
    set(RealSources "")
    set(UnitySources "")
    foreach(UnitySource ${UnityInput})
      get_property(SourceExcluded SOURCE ${UnitySource} PROPERTY EXCLUDE_UNITY_BUILD)
      if(SourceExcluded MATCHES TRUE OR NOT ${UnitySource} MATCHES "\\.(cpp|cxx|cc)\$")
          list(APPEND RealSources ${UnitySource})
      else()
          list(APPEND UnitySources ${UnitySource})
      endif()
    endforeach()
    add_custom_command(
      OUTPUT ${UnityOutput}
      COMMAND mkunity
      ARGS ${Prefix} ${UnitySources} > ${UnityOutput}.tmp
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${UnityOutput}.tmp ${UnityOutput}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS mkunity ${UnitySources}
    )
    list(APPEND RealSources ${UnityOutput})
    set(${UnityInputRef} ${RealSources} PARENT_SCOPE)
  endfunction()
endif()
