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

mkclass_target(checkresultreader.ti checkresultreader.tcpp checkresultreader.thpp)
mkclass_target(compatlogger.ti compatlogger.tcpp compatlogger.thpp)
mkclass_target(externalcommandlistener.ti externalcommandlistener.tcpp externalcommandlistener.thpp)
mkclass_target(statusdatawriter.ti statusdatawriter.tcpp statusdatawriter.thpp)

set(compat_SOURCES
  checkresultreader.cpp checkresultreader.thpp compatlogger.cpp
  compatlogger.thpp externalcommandlistener.cpp externalcommandlistener.thpp
  statusdatawriter.cpp statusdatawriter.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(compat compat compat_SOURCES)
endif()

add_library(compat SHARED ${compat_SOURCES})

target_link_libraries(compat ${Boost_LIBRARIES} base config icinga)

set_target_properties (
  compat PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_COMPAT_BUILD
  FOLDER Components
  VERSION ${SPEC_VERSION}
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/command.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/compatlog.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install_if_not_exists(
  ${PROJECT_SOURCE_DIR}/etc/icinga2/features-available/statusdata.conf
  ${CMAKE_INSTALL_SYSCONFDIR}/icinga2/features-available
)

install(TARGETS compat RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2)

install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/icinga2/compat/archives\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/spool/icinga2\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_RUNDIR}/icinga2/cmd\")")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
