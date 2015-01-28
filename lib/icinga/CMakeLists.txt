# Icinga 2
# Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)
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

mkclass_target(checkable.ti checkable.thpp)
mkclass_target(checkcommand.ti checkcommand.thpp)
mkclass_target(checkresult.ti checkresult.thpp)
mkclass_target(command.ti command.thpp)
mkclass_target(comment.ti comment.thpp)
mkclass_target(dependency.ti dependency.thpp)
mkclass_target(downtime.ti downtime.thpp)
mkclass_target(eventcommand.ti eventcommand.thpp)
mkclass_target(hostgroup.ti hostgroup.thpp)
mkclass_target(host.ti host.thpp)
mkclass_target(icingaapplication.ti icingaapplication.thpp)
mkclass_target(customvarobject.ti customvarobject.thpp)
mkclass_target(icingastatuswriter.ti icingastatuswriter.thpp)
mkclass_target(notificationcommand.ti notificationcommand.thpp)
mkclass_target(notification.ti notification.thpp)
mkclass_target(perfdatavalue.ti perfdatavalue.thpp)
mkclass_target(scheduleddowntime.ti scheduleddowntime.thpp)
mkclass_target(servicegroup.ti servicegroup.thpp)
mkclass_target(service.ti service.thpp)
mkclass_target(timeperiod.ti timeperiod.thpp)
mkclass_target(usergroup.ti usergroup.thpp)
mkclass_target(user.ti user.thpp)

mkembedconfig_target(icinga-type.conf icinga-type.cpp)

set(icinga_SOURCES
  api.cpp apievents.cpp checkable.cpp checkable.thpp checkable-dependency.cpp checkable-downtime.cpp checkable-event.cpp
  checkable-flapping.cpp checkcommand.cpp checkcommand.thpp checkresult.cpp checkresult.thpp
  cib.cpp command.cpp command.thpp comment.cpp comment.thpp compatutility.cpp dependency.cpp dependency.thpp
  dependency-apply.cpp downtime.cpp downtime.thpp eventcommand.cpp eventcommand.thpp
  externalcommandprocessor.cpp host.cpp host.thpp hostgroup.cpp hostgroup.thpp icingaapplication.cpp
  icingaapplication.thpp customvarobject.cpp customvarobject.thpp icingastatuswriter.cpp icingastatuswriter.thpp
  legacytimeperiod.cpp macroprocessor.cpp notificationcommand.cpp notificationcommand.thpp notification.cpp
  notification.thpp notification-apply.cpp objectutils.cpp perfdatavalue.cpp perfdatavalue.thpp pluginutility.cpp scheduleddowntime.cpp
  scheduleddowntime.thpp scheduleddowntime-apply.cpp service-apply.cpp checkable-check.cpp checkable-comment.cpp
  service.cpp service.thpp servicegroup.cpp servicegroup.thpp checkable-notification.cpp timeperiod.cpp timeperiod.thpp
  user.cpp user.thpp usergroup.cpp usergroup.thpp icinga-type.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(icinga icinga_SOURCES)
endif()

add_library(icinga SHARED ${icinga_SOURCES})

target_link_libraries(icinga ${Boost_LIBRARIES} base config remote)

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


