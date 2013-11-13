# Icinga 2
# Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)
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

mkclass_target(checkcommand.ti checkcommand.th)
mkclass_target(checkresult.ti checkresult.th)
mkclass_target(command.ti command.th)
mkclass_target(comment.ti comment.th)
mkclass_target(domain.ti domain.th)
mkclass_target(downtime.ti downtime.th)
mkclass_target(eventcommand.ti eventcommand.th)
mkclass_target(hostgroup.ti hostgroup.th)
mkclass_target(host.ti host.th)
mkclass_target(icingaapplication.ti icingaapplication.th)
mkclass_target(notificationcommand.ti notificationcommand.th)
mkclass_target(notification.ti notification.th)
mkclass_target(perfdatavalue.ti perfdatavalue.th)
mkclass_target(scheduleddowntime.ti scheduleddowntime.th)
mkclass_target(servicegroup.ti servicegroup.th)
mkclass_target(service.ti service.th)
mkclass_target(timeperiod.ti timeperiod.th)
mkclass_target(usergroup.ti usergroup.th)
mkclass_target(user.ti user.th)

mkembedconfig_target(icinga-type.conf icinga-type.cpp)

add_library(icinga SHARED
  api.cpp api.h checkcommand.cpp checkcommand.th checkresult.cpp checkresult.th
  cib.cpp command.cpp command.th comment.cpp comment.th compatutility.cpp
  domain.cpp domain.th downtime.cpp downtime.th eventcommand.cpp eventcommand.th
  externalcommandprocessor.cpp host.cpp host.th hostgroup.cpp hostgroup.th
  icingaapplication.cpp icingaapplication.th legacytimeperiod.cpp
  macroprocessor.cpp macroresolver.cpp notificationcommand.cpp notificationcommand.th
  notification.cpp notification.th perfdatavalue.cpp perfdatavalue.th
  pluginutility.cpp scheduleddowntime.cpp scheduleddowntime.th service-check.cpp
  service-comment.cpp service.cpp service-downtime.cpp service-event.cpp
  service-flapping.cpp service.th servicegroup.cpp servicegroup.th
  service-notification.cpp timeperiod.cpp timeperiod.th user.cpp user.th
  usergroup.cpp usergroup.th icinga-type.cpp
)

target_link_libraries(icinga ${Boost_LIBRARIES} base config)

set_target_properties (
  icinga PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_ICINGA_BUILD
  FOLDER Lib
)

install(
  TARGETS icinga
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)


