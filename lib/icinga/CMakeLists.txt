# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkembedconfig_target(icinga-itl.conf icinga-itl.cpp)

add_library(icinga OBJECT
  i2-icinga.hpp icinga-itl.cpp
  apiactions.cpp apiactions.hpp
  apievents.cpp apievents.hpp
  checkable.cpp checkable.hpp checkable.ti
  checkable-check.cpp checkable-comment.cpp checkable-dependency.cpp
  checkable-downtime.cpp checkable-event.cpp checkable-flapping.cpp
  checkable-notification.cpp
  checkcommand.cpp checkcommand.hpp checkcommand.ti
  checkresult.cpp checkresult.hpp checkresult.ti
  cib.cpp cib.hpp
  clusterevents.cpp clusterevents.hpp clusterevents-check.cpp
  command.cpp command.hpp command.ti
  comment.cpp comment.hpp comment.ti
  compatutility.cpp compatutility.hpp
  customvarobject.cpp customvarobject.hpp customvarobject.ti
  dependency.cpp dependency-group.cpp dependency-state.cpp dependency.hpp dependency.ti dependency-apply.cpp
  downtime.cpp downtime.hpp downtime.ti
  envresolver.cpp envresolver.hpp
  eventcommand.cpp eventcommand.hpp eventcommand.ti
  externalcommandprocessor.cpp externalcommandprocessor.hpp
  host.cpp host.hpp host.ti
  hostgroup.cpp hostgroup.hpp hostgroup.ti
  icingaapplication.cpp icingaapplication.hpp icingaapplication.ti
  legacytimeperiod.cpp legacytimeperiod.hpp
  macroprocessor.cpp macroprocessor.hpp
  macroresolver.hpp
  notification.cpp notification.hpp notification.ti notification-apply.cpp
  notificationcommand.cpp notificationcommand.hpp notificationcommand.ti
  objectutils.cpp objectutils.hpp
  pluginutility.cpp pluginutility.hpp
  scheduleddowntime.cpp scheduleddowntime.hpp scheduleddowntime.ti scheduleddowntime-apply.cpp
  service.cpp service.hpp service.ti service-apply.cpp
  servicegroup.cpp servicegroup.hpp servicegroup.ti
  timeperiod.cpp timeperiod.hpp timeperiod.ti
  user.cpp user.hpp user.ti
  usergroup.cpp usergroup.hpp usergroup.ti
)

target_link_libraries(icinga PUBLIC
  common
)

target_mkclass(icinga icinga_generated)

add_dependencies(icinga base_generated remote_generated)

set_target_properties (
  icinga PROPERTIES
  FOLDER Lib
)
