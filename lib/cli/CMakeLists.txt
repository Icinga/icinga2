# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(cli OBJECT
  i2-cli.hpp
  apisetupcommand.cpp apisetupcommand.hpp
  apisetuputility.cpp apisetuputility.hpp
  calistcommand.cpp calistcommand.hpp
  caremovecommand.cpp caremovecommand.hpp
  carestorecommand.cpp carestorecommand.hpp
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
  pkiverifycommand.cpp pkiverifycommand.hpp
  variablegetcommand.cpp variablegetcommand.hpp
  variablelistcommand.cpp variablelistcommand.hpp
  variableutility.cpp variableutility.hpp
)

target_link_libraries(cli PUBLIC
  common
)

add_dependencies(cli base_generated icinga_generated remote_generated)

if(Editline_FOUND)
  target_link_libraries(cli PUBLIC Editline)
endif()

set_target_properties (
  cli PROPERTIES
  FOLDER Lib
)
