# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

mkclass_target(checkable.ti checkable-ti.cpp checkable-ti.hpp)
mkclass_target(checkcommand.ti checkcommand-ti.cpp checkcommand-ti.hpp)
mkclass_target(checkresult.ti checkresult-ti.cpp checkresult-ti.hpp)
mkclass_target(command.ti command-ti.cpp command-ti.hpp)
mkclass_target(comment.ti comment-ti.cpp comment-ti.hpp)
mkclass_target(dependency.ti dependency-ti.cpp dependency-ti.hpp)
mkclass_target(downtime.ti downtime-ti.cpp downtime-ti.hpp)
mkclass_target(eventcommand.ti eventcommand-ti.cpp eventcommand-ti.hpp)
mkclass_target(hostgroup.ti hostgroup-ti.cpp hostgroup-ti.hpp)
mkclass_target(host.ti host-ti.cpp host-ti.hpp)
mkclass_target(icingaapplication.ti icingaapplication-ti.cpp icingaapplication-ti.hpp)
mkclass_target(customvarobject.ti customvarobject-ti.cpp customvarobject-ti.hpp)
mkclass_target(notificationcommand.ti notificationcommand-ti.cpp notificationcommand-ti.hpp)
mkclass_target(notification.ti notification-ti.cpp notification-ti.hpp)
mkclass_target(scheduleddowntime.ti scheduleddowntime-ti.cpp scheduleddowntime-ti.hpp)
mkclass_target(servicegroup.ti servicegroup-ti.cpp servicegroup-ti.hpp)
mkclass_target(service.ti service-ti.cpp service-ti.hpp)
mkclass_target(timeperiod.ti timeperiod-ti.cpp timeperiod-ti.hpp)
mkclass_target(usergroup.ti usergroup-ti.cpp usergroup-ti.hpp)
mkclass_target(user.ti user-ti.cpp user-ti.hpp)

mkembedconfig_target(icinga-itl.conf icinga-itl.cpp)

set(icinga_SOURCES
  i2-icinga.hpp icinga-itl.cpp
  apiactions.cpp apiactions.hpp
  apievents.cpp apievents.hpp
  checkable.cpp checkable.hpp checkable-ti.hpp
  checkable-check.cpp checkable-comment.cpp checkable-dependency.cpp
  checkable-downtime.cpp checkable-event.cpp checkable-flapping.cpp
  checkable-notification.cpp
  checkcommand.cpp checkcommand.hpp checkcommand-ti.hpp
  checkresult.cpp checkresult.hpp checkresult-ti.hpp
  cib.cpp cib.hpp
  clusterevents.cpp clusterevents.hpp clusterevents-check.cpp
  command.cpp command.hpp command-ti.hpp
  comment.cpp comment.hpp comment-ti.hpp
  compatutility.cpp compatutility.hpp
  customvarobject.cpp customvarobject.hpp customvarobject-ti.hpp
  dependency.cpp dependency-group.cpp dependency-state.cpp dependency.hpp dependency-ti.hpp dependency-apply.cpp
  downtime.cpp downtime.hpp downtime-ti.hpp
  envresolver.cpp envresolver.hpp
  eventcommand.cpp eventcommand.hpp eventcommand-ti.hpp
  externalcommandprocessor.cpp externalcommandprocessor.hpp
  host.cpp host.hpp host-ti.hpp
  hostgroup.cpp hostgroup.hpp hostgroup-ti.hpp
  icingaapplication.cpp icingaapplication.hpp icingaapplication-ti.hpp
  legacytimeperiod.cpp legacytimeperiod.hpp
  macroprocessor.cpp macroprocessor.hpp
  macroresolver.hpp
  notification.cpp notification.hpp notification-ti.hpp notification-apply.cpp
  notificationcommand.cpp notificationcommand.hpp notificationcommand-ti.hpp
  objectutils.cpp objectutils.hpp
  pluginutility.cpp pluginutility.hpp
  scheduleddowntime.cpp scheduleddowntime.hpp scheduleddowntime-ti.hpp scheduleddowntime-apply.cpp
  service.cpp service.hpp service-ti.hpp service-apply.cpp
  servicegroup.cpp servicegroup.hpp servicegroup-ti.hpp
  timeperiod.cpp timeperiod.hpp timeperiod-ti.hpp
  user.cpp user.hpp user-ti.hpp
  usergroup.cpp usergroup.hpp usergroup-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(icinga icinga icinga_SOURCES)
endif()

add_library(icinga OBJECT ${icinga_SOURCES})

add_dependencies(icinga base config remote)

set_target_properties (
  icinga PROPERTIES
  FOLDER Lib
)
