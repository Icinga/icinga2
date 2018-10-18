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

mkclass_target(checkresultreader.ti checkresultreader-ti.cpp checkresultreader-ti.hpp)
mkclass_target(compatlogger.ti compatlogger-ti.cpp compatlogger-ti.hpp)
mkclass_target(externalcommandlistener.ti externalcommandlistener-ti.cpp externalcommandlistener-ti.hpp)
mkclass_target(statusdatawriter.ti statusdatawriter-ti.cpp statusdatawriter-ti.hpp)

set(compat_SOURCES
  checkresultreader.cpp checkresultreader.hpp checkresultreader-ti.hpp
  compatlogger.cpp compatlogger.hpp compatlogger-ti.hpp
  externalcommandlistener.cpp externalcommandlistener.hpp externalcommandlistener-ti.hpp
  statusdatawriter.cpp statusdatawriter.hpp statusdatawriter-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(compat compat compat_SOURCES)
endif()

add_library(compat OBJECT ${compat_SOURCES})

add_dependencies(compat base config icinga)

set_target_properties (
  compat PROPERTIES
  FOLDER Components
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/command.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/compatlog.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/statusdata.conf
  ${ICINGA2_CONFIGDIR}/features-available
)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_LOGDIR}/compat/archives\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_SPOOLDIR}\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_INITRUNDIR}/cmd\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
