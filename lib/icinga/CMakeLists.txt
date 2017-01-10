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

mkclass_target(checkable.ti checkable.tcpp checkable.thpp)
mkclass_target(checkcommand.ti checkcommand.tcpp checkcommand.thpp)
mkclass_target(checkresult.ti checkresult.tcpp checkresult.thpp)
mkclass_target(command.ti command.tcpp command.thpp)
mkclass_target(comment.ti comment.tcpp comment.thpp)
mkclass_target(dependency.ti dependency.tcpp dependency.thpp)
mkclass_target(downtime.ti downtime.tcpp downtime.thpp)
mkclass_target(eventcommand.ti eventcommand.tcpp eventcommand.thpp)
mkclass_target(hostgroup.ti hostgroup.tcpp hostgroup.thpp)
mkclass_target(host.ti host.tcpp host.thpp)
mkclass_target(icingaapplication.ti icingaapplication.tcpp icingaapplication.thpp)
mkclass_target(customvarobject.ti customvarobject.tcpp customvarobject.thpp)
mkclass_target(notificationcommand.ti notificationcommand.tcpp notificationcommand.thpp)
mkclass_target(notification.ti notification.tcpp notification.thpp)
mkclass_target(perfdatavalue.ti perfdatavalue.tcpp perfdatavalue.thpp)
mkclass_target(scheduleddowntime.ti scheduleddowntime.tcpp scheduleddowntime.thpp)
mkclass_target(servicegroup.ti servicegroup.tcpp servicegroup.thpp)
mkclass_target(service.ti service.tcpp service.thpp)
mkclass_target(timeperiod.ti timeperiod.tcpp timeperiod.thpp)
mkclass_target(usergroup.ti usergroup.tcpp usergroup.thpp)
mkclass_target(user.ti user.tcpp user.thpp)

mkembedconfig_target(icinga-itl.conf icinga-itl.cpp)

set(icinga_SOURCES
  apiactions.cpp apievents.cpp checkable.cpp checkable.thpp checkable-dependency.cpp checkable-downtime.cpp checkable-event.cpp
  checkable-flapping.cpp checkable-script.cpp checkcommand.cpp checkcommand.thpp checkresult.cpp checkresult.thpp
  cib.cpp clusterevents.cpp command.cpp command.thpp comment.cpp comment.thpp compatutility.cpp dependency.cpp dependency.thpp
  dependency-apply.cpp downtime.cpp downtime.thpp eventcommand.cpp eventcommand.thpp
  externalcommandprocessor.cpp host.cpp host.thpp hostgroup.cpp hostgroup.thpp icingaapplication.cpp icingaapplication.thpp
  icinga-itl.cpp customvarobject.cpp customvarobject.thpp
  legacytimeperiod.cpp macroprocessor.cpp notificationcommand.cpp notificationcommand.thpp notification.cpp notification.thpp
  notification-apply.cpp objectutils.cpp perfdatavalue.cpp perfdatavalue.thpp pluginutility.cpp scheduleddowntime.cpp scheduleddowntime.thpp
  scheduleddowntime-apply.cpp service-apply.cpp checkable-check.cpp checkable-comment.cpp
  service.cpp service.thpp servicegroup.cpp servicegroup.thpp checkable-notification.cpp timeperiod.cpp timeperiod.thpp
  user.cpp user.thpp usergroup.cpp usergroup.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(icinga icinga icinga_SOURCES)
endif()

add_library(icinga SHARED ${icinga_SOURCES})

target_link_libraries(icinga ${Boost_LIBRARIES} base config remote)

set_target_properties (
  icinga PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_ICINGA_BUILD
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS icinga
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)


