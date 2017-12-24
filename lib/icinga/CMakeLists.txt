# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)
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
mkclass_target(scheduleddowntime.ti scheduleddowntime.tcpp scheduleddowntime.thpp)
mkclass_target(servicegroup.ti servicegroup.tcpp servicegroup.thpp)
mkclass_target(service.ti service.tcpp service.thpp)
mkclass_target(timeperiod.ti timeperiod.tcpp timeperiod.thpp)
mkclass_target(usergroup.ti usergroup.tcpp usergroup.thpp)
mkclass_target(user.ti user.tcpp user.thpp)

mkembedconfig_target(icinga-itl.conf icinga-itl.cpp)

set(icinga_SOURCES
  i2-icinga.hpp icinga-itl.cpp
  apiactions.cpp apiactions.hpp
  apievents.cpp apievents.hpp
  checkable.cpp checkable.hpp checkable.thpp
  checkable-check.cpp checkable-comment.cpp checkable-dependency.cpp
  checkable-downtime.cpp checkable-event.cpp checkable-flapping.cpp
  checkable-notification.cpp checkable-script.cpp
  checkcommand.cpp checkcommand.hpp checkcommand.thpp
  checkresult.cpp checkresult.hpp checkresult.thpp
  cib.cpp cib.hpp
  clusterevents.cpp clusterevents.hpp
  command.cpp command.hpp command.thpp
  comment.cpp comment.hpp comment.thpp
  compatutility.cpp compatutility.hpp
  customvarobject.cpp customvarobject.hpp customvarobject.thpp
  dependency.cpp dependency.hpp dependency.thpp dependency-apply.cpp
  downtime.cpp downtime.hpp downtime.thpp
  eventcommand.cpp eventcommand.hpp eventcommand.thpp
  externalcommandprocessor.cpp externalcommandprocessor.hpp
  host.cpp host.hpp host.thpp
  hostgroup.cpp hostgroup.hpp hostgroup.thpp
  icingaapplication.cpp icingaapplication.hpp icingaapplication.thpp
  legacytimeperiod.cpp legacytimeperiod.hpp
  macroprocessor.cpp macroprocessor.hpp
  macroresolver.hpp
  notification.cpp notification.hpp notification.thpp notification-apply.cpp
  notificationcommand.cpp notificationcommand.hpp notificationcommand.thpp
  objectutils.cpp objectutils.hpp
  pluginutility.cpp pluginutility.hpp
  scheduleddowntime.cpp scheduleddowntime.hpp scheduleddowntime.thpp scheduleddowntime-apply.cpp
  service.cpp service.hpp service.thpp service-apply.cpp
  servicegroup.cpp servicegroup.hpp servicegroup.thpp
  timeperiod.cpp timeperiod.hpp timeperiod.thpp
  user.cpp user.hpp user.thpp
  usergroup.cpp usergroup.hpp usergroup.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(icinga icinga icinga_SOURCES)
endif()

add_library(icinga STATIC ${icinga_SOURCES})

target_link_libraries(icinga ${Boost_LIBRARIES} base config remote)

set_target_properties (
  icinga PROPERTIES
  FOLDER Lib
)

if(ICINGA2_PCH_BUILD)
  target_precompiled_header(icinga pch.hpp REUSE remote)
endif()
