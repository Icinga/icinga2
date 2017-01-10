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

set(cli_SOURCES
  apisetupcommand.cpp apisetuputility.cpp
  nodeaddcommand.cpp nodeblackandwhitelistcommand.cpp nodelistcommand.cpp noderemovecommand.cpp
  nodesetcommand.cpp nodesetupcommand.cpp nodeupdateconfigcommand.cpp nodewizardcommand.cpp nodeutility.cpp
  clicommand.cpp
  consolecommand.cpp
  daemoncommand.cpp daemonutility.cpp
  featureenablecommand.cpp featuredisablecommand.cpp featurelistcommand.cpp featureutility.cpp
  objectlistcommand.cpp objectlistutility.cpp
  pkinewcacommand.cpp pkinewcertcommand.cpp pkisigncsrcommand.cpp pkirequestcommand.cpp pkisavecertcommand.cpp pkiticketcommand.cpp
  pkiutility.cpp
  repositoryclearchangescommand.cpp repositorycommitcommand.cpp repositoryobjectcommand.cpp repositoryutility.cpp
  variablegetcommand.cpp variablelistcommand.cpp variableutility.cpp
  troubleshootcommand.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(cli cli cli_SOURCES)
endif()

add_library(cli SHARED ${cli_SOURCES})

target_link_libraries(cli ${Boost_LIBRARIES} base config remote)

if(EDITLINE_FOUND)
  target_link_libraries(cli ${EDITLINE_LIBRARIES})
  include_directories(${EDITLINE_INCLUDE_DIR})
endif()

if(TERMCAP_FOUND)
  target_link_libraries(cli ${TERMCAP_LIBRARIES})
  include_directories(${TERMCAP_INCLUDE_DIR})
endif()

set_target_properties (
  cli PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_CLI_BUILD
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS cli
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)
