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

mkembedconfig_target(methods-itl.conf methods-itl.cpp)

if(MSVC)
  if(ICINGA2_WITH_CLRCHECKTASK)
    set(WindowsSources clrchecktask.cpp)
  else()
    set(WindowsSources "")
  endif()
else()
  set(WindowsSources "")
endif()

set(methods_SOURCES
  clusterchecktask.cpp clusterzonechecktask.cpp exceptionchecktask.cpp
  icingachecktask.cpp methods-itl.cpp nullchecktask.cpp nulleventtask.cpp
  pluginchecktask.cpp plugineventtask.cpp pluginnotificationtask.cpp
  randomchecktask.cpp timeperiodtask.cpp ${WindowsSources}
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(methods methods methods_SOURCES)
endif()

add_library(methods SHARED ${methods_SOURCES})

target_link_libraries(methods ${Boost_LIBRARIES} base config icinga)

set_target_properties (
  methods PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_METHODS_BUILD
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS methods
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)


