# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
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

foreach(flag_var
  CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
  CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

set(icinga_installer_SOURCES
  icinga-installer.cpp
)

add_executable(icinga-installer ${icinga_installer_SOURCES})

set_target_properties(
  icinga-installer PROPERTIES
  FOLDER Bin
  OUTPUT_NAME icinga2-installer
  LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

target_link_libraries(icinga-installer shlwapi)

install(CODE "
  execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${CMAKE_CURRENT_BINARY_DIR}/icinga2.wixpatch.\${BUILD_TYPE}\"
    \"${CMAKE_CURRENT_BINARY_DIR}/icinga2.wixpatch\"
    RESULT_VARIABLE copy_result
    ERROR_VARIABLE error_output)
  if(copy_result)
    message(FATAL_ERROR \${error_output})
  endif()"
)

file(
  GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/icinga2.wixpatch.$<CONFIG>"
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/icinga2.wixpatch.cmake"
)

set(InstallPath "${CMAKE_INSTALL_SBINDIR}")

install(
  TARGETS icinga-installer
  RUNTIME DESTINATION ${InstallPath}
)
