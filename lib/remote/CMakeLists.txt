# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkclass_target(apilistener.ti apilistener-ti.cpp apilistener-ti.hpp)
mkclass_target(apiuser.ti apiuser-ti.cpp apiuser-ti.hpp)
mkclass_target(endpoint.ti endpoint-ti.cpp endpoint-ti.hpp)
mkclass_target(zone.ti zone-ti.cpp zone-ti.hpp)

set(remote_SOURCES
  i2-remote.hpp
  actionshandler.cpp actionshandler.hpp
  apiaction.cpp apiaction.hpp
  apifunction.cpp apifunction.hpp
  apilistener.cpp apilistener.hpp apilistener-ti.hpp apilistener-configsync.cpp apilistener-filesync.cpp
  apilistener-authority.cpp
  apiuser.cpp apiuser.hpp apiuser-ti.hpp
  configfileshandler.cpp configfileshandler.hpp
  configobjectslock.cpp configobjectslock.hpp
  configobjectutility.cpp configobjectutility.hpp
  configpackageshandler.cpp configpackageshandler.hpp
  configpackageutility.cpp configpackageutility.hpp
  configstageshandler.cpp configstageshandler.hpp
  consolehandler.cpp consolehandler.hpp
  createobjecthandler.cpp createobjecthandler.hpp
  deleteobjecthandler.cpp deleteobjecthandler.hpp
  endpoint.cpp endpoint.hpp endpoint-ti.hpp
  eventqueue.cpp eventqueue.hpp
  eventshandler.cpp eventshandler.hpp
  filterutility.cpp filterutility.hpp
  httphandler.cpp httphandler.hpp
  httpmessage.cpp httpmessage.hpp
  httpserverconnection.cpp httpserverconnection.hpp
  httputility.cpp httputility.hpp
  infohandler.cpp infohandler.hpp
  jsonrpc.cpp jsonrpc.hpp
  jsonrpcconnection.cpp jsonrpcconnection.hpp jsonrpcconnection-heartbeat.cpp jsonrpcconnection-pki.cpp
  mallocinfohandler.cpp mallocinfohandler.hpp
  messageorigin.cpp messageorigin.hpp
  modifyobjecthandler.cpp modifyobjecthandler.hpp
  objectqueryhandler.cpp objectqueryhandler.hpp
  pkiutility.cpp pkiutility.hpp
  statushandler.cpp statushandler.hpp
  templatequeryhandler.cpp templatequeryhandler.hpp
  typequeryhandler.cpp typequeryhandler.hpp
  url.cpp url.hpp url-characters.hpp
  variablequeryhandler.cpp variablequeryhandler.hpp
  zone.cpp zone.hpp zone-ti.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(remote remote remote_SOURCES)
endif()

add_library(remote OBJECT ${remote_SOURCES})

add_dependencies(remote base config)

set_target_properties (
  remote PROPERTIES
  FOLDER Lib
)

if(NOT WIN32)
  #install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}/api\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}/api/log\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}/api/zones\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}/api/zones-stage\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}/certs\")")
  install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${ICINGA2_FULL_DATADIR}/certificate-requests\")")
endif()
