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

mkclass_target(apilistener.ti apilistener-ti.cpp apilistener-ti.hpp)
mkclass_target(apiuser.ti apiuser-ti.cpp apiuser-ti.hpp)
mkclass_target(endpoint.ti endpoint-ti.cpp endpoint-ti.hpp)
mkclass_target(zone.ti zone-ti.cpp zone-ti.hpp)

set(remote_SOURCES
  i2-remote.hpp
  actionshandler.cpp actionshandler.hpp
  apiaction.cpp apiaction.hpp
  apiclient.cpp apiclient.hpp
  apifunction.cpp apifunction.hpp
  apilistener.cpp apilistener.hpp apilistener-ti.hpp apilistener-configsync.cpp apilistener-filesync.cpp
  apiuser.cpp apiuser.hpp apiuser-ti.hpp
  authority.cpp
  configfileshandler.cpp configfileshandler.hpp
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
  httpchunkedencoding.cpp httpchunkedencoding.hpp
  httpclientconnection.cpp httpclientconnection.hpp
  httphandler.cpp httphandler.hpp
  httprequest.cpp httprequest.hpp
  httpresponse.cpp httpresponse.hpp
  httpserverconnection.cpp httpserverconnection.hpp
  httputility.cpp httputility.hpp
  infohandler.cpp infohandler.hpp
  jsonrpc.cpp jsonrpc.hpp
  jsonrpcconnection.cpp jsonrpcconnection.hpp jsonrpcconnection-heartbeat.cpp jsonrpcconnection-pki.cpp
  messageorigin.cpp messageorigin.hpp
  modifyobjecthandler.cpp modifyobjecthandler.hpp
  objectqueryhandler.cpp objectqueryhandler.hpp
  pkiutility.cpp pkiutility.hpp
  statsreporter.cpp statsreporter.hpp
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

#install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/log\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/zones\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/certs\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/certificate-requests\")")


