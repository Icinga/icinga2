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

set(cli_SOURCES
  i2-cli.hpp
  apisetupcommand.cpp apisetupcommand.hpp
  apisetuputility.cpp apisetuputility.hpp
  calistcommand.cpp calistcommand.hpp
  casigncommand.cpp casigncommand.hpp
  clicommand.cpp clicommand.hpp
  consolecommand.cpp consolecommand.hpp
  daemoncommand.cpp daemoncommand.hpp
  daemonutility.cpp daemonutility.hpp
  editline.hpp
  featuredisablecommand.cpp featuredisablecommand.hpp
  featureenablecommand.cpp featureenablecommand.hpp
  featurelistcommand.cpp featurelistcommand.hpp
  featureutility.cpp featureutility.hpp
  internalsignalcommand.cpp internalsignalcommand.hpp
  nodesetupcommand.cpp nodesetupcommand.hpp
  nodeutility.cpp nodeutility.hpp
  nodewizardcommand.cpp nodewizardcommand.hpp
  objectlistcommand.cpp objectlistcommand.hpp
  objectlistutility.cpp objectlistutility.hpp
  pkinewcacommand.cpp pkinewcacommand.hpp
  pkinewcertcommand.cpp pkinewcertcommand.hpp
  pkirequestcommand.cpp pkirequestcommand.hpp
  pkisavecertcommand.cpp pkisavecertcommand.hpp
  pkisigncsrcommand.cpp pkisigncsrcommand.hpp
  pkiticketcommand.cpp pkiticketcommand.hpp
  troubleshootcommand.cpp troubleshootcommand.hpp
  variablegetcommand.cpp variablegetcommand.hpp
  variablelistcommand.cpp variablelistcommand.hpp
  variableutility.cpp variableutility.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(cli cli cli_SOURCES)
endif()

add_library(cli OBJECT ${cli_SOURCES})

add_dependencies(cli base config remote)

set_target_properties (
  cli PROPERTIES
  FOLDER Lib
)
