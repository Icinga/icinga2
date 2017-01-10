# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
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

set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base propgrid adv REQUIRED)
include(${wxWidgets_USE_FILE})

if(MSVC)
  set(WindowsSources icinga.rc)
else()
  set(WindowsSources "")
endif()

add_executable(icinga-studio MACOSX_BUNDLE WIN32 icinga-studio.cpp
  forms.cpp aboutform.cpp connectform.cpp mainform.cpp
  icinga.icns ${WindowsSources})

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(icinga-studio ${Boost_LIBRARIES} ${wxWidgets_LIBRARIES} base remote)

if(APPLE)
  set_source_files_properties(icinga.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

set_target_properties (
  icinga-studio PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  FOLDER Bin
  OUTPUT_NAME icinga-studio
  MACOSX_BUNDLE_INFO_STRING "Icinga Studio"
  MACOSX_BUNDLE_BUNDLE_NAME "Icinga Studio"
  MACOSX_BUNDLE_GUI_IDENTIFIER "Icinga Studio"
  MACOSX_BUNDLE_ICON_FILE icinga.icns
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${GIT_VERSION}"
  MACOSX_BUNDLE_LONG_VERSION_STRING "${GIT_VERSION}"
  MACOSX_BUNDLE_COPYRIGHT "(c) Icinga Development Team"
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
)

if(WIN32)
  set(InstallPath "${CMAKE_INSTALL_SBINDIR}")
else()
  set(InstallPath "${CMAKE_INSTALL_BINDIR}")
endif()

install(
  TARGETS icinga-studio
  RUNTIME DESTINATION ${InstallPath}
  BUNDLE DESTINATION ${InstallPath}
)

