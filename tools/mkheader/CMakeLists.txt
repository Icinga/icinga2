# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

define_property(
  SOURCE
  PROPERTY EXCLUDE_UNITY_BUILD
  BRIEF_DOCS "Whether to exclude the source file from unity builds"
  FULL_DOCS "Specified whether a source file should be excluded from unity builds and should be built separately"
)

if(ICINGA2_UNITY_BUILD OR ICINGA2_PCH_BUILD)
    set(mkheader_SOURCES
      mkheader.cpp
    )

    add_executable(mkheader ${mkheader_SOURCES})

    set_target_properties(
      mkheader PROPERTIES
      FOLDER Bin
    )
endif()

if(ICINGA2_UNITY_BUILD)
    include(ProcessorCount)
    ProcessorCount(processors)
    set(ICINGA2_UNITY_FRAGMENTS ${processors} CACHE STRING "The number of unity fragments")

    function(target_unity_build Target Prefix UnityInputRef)
        set(UnityInput ${${UnityInputRef}})
        set(UnityOutputTemplate ${CMAKE_CURRENT_BINARY_DIR}/${Target}_unity_{0}.cpp)
        set(RealSources "")
        set(UnitySources "")
        foreach(UnitySource ${UnityInput})
            get_property(SourceExcluded SOURCE ${UnitySource} PROPERTY EXCLUDE_UNITY_BUILD)
            if(SourceExcluded MATCHES TRUE OR NOT ${UnitySource} MATCHES "\\.(cpp|cxx|cc)\$")
                list(APPEND RealSources ${UnitySource})
            else()
                list(APPEND UnitySources ${Prefix}/${UnitySource})
            endif()
        endforeach()
        list(LENGTH UnitySources UnitySourceCount)
        set(UnityFragmentMax "${ICINGA2_UNITY_FRAGMENTS}")
        if(UnitySourceCount GREATER "${UnityFragmentMax}")
            math(EXPR UnityFragmentCount "${UnitySourceCount} / ${UnityFragmentMax}")
            if(UnityFragmentCount GREATER ${UnityFragmentMax})
                set(UnityFragmentCount ${UnityFragmentMax})
            endif()

            set(UnityOutput "")
            math(EXPR UnityFragmentMaxIndex "${UnityFragmentCount} - 1")
            foreach(CurrentFragment RANGE ${UnityFragmentMaxIndex})
                set(Fragment ${UnityOutputTemplate})
                string(REPLACE "{0}" ${CurrentFragment} Fragment ${Fragment})
                list(APPEND UnityOutput ${Fragment})
                list(APPEND RealSources ${Fragment})
            endforeach()
            add_custom_command(
                OUTPUT ${UnityOutput}
                COMMAND mkheader
                ARGS ${UnityFragmentCount} ${UnityOutputTemplate} ${UnitySources}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS mkheader
            )
            set(${UnityInputRef} ${RealSources} PARENT_SCOPE)
        endif()
    endfunction()
endif()
