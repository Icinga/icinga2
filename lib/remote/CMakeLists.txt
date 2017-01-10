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

mkclass_target(apilistener.ti apilistener.tcpp apilistener.thpp)
mkclass_target(apiuser.ti apiuser.tcpp apiuser.thpp)
mkclass_target(endpoint.ti endpoint.tcpp endpoint.thpp)
mkclass_target(zone.ti zone.tcpp zone.thpp)

set(remote_SOURCES
  actionshandler.cpp apiaction.cpp apiclient.cpp
  apifunction.cpp apilistener.cpp apilistener.thpp apilistener-configsync.cpp
  apilistener-filesync.cpp apiuser.cpp apiuser.thpp authority.cpp base64.cpp
  consolehandler.cpp configfileshandler.cpp configpackageshandler.cpp configpackageutility.cpp configobjectutility.cpp
  configstageshandler.cpp createobjecthandler.cpp deleteobjecthandler.cpp
  endpoint.cpp endpoint.thpp eventshandler.cpp eventqueue.cpp filterutility.cpp
  httpchunkedencoding.cpp httpclientconnection.cpp httpserverconnection.cpp httphandler.cpp httprequest.cpp httpresponse.cpp
  httputility.cpp infohandler.cpp jsonrpc.cpp jsonrpcconnection.cpp jsonrpcconnection-heartbeat.cpp
  messageorigin.cpp modifyobjecthandler.cpp statushandler.cpp objectqueryhandler.cpp templatequeryhandler.cpp
  typequeryhandler.cpp url.cpp variablequeryhandler.cpp zone.cpp zone.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(remote remote remote_SOURCES)
endif()

add_library(remote SHARED ${remote_SOURCES})

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(remote ${Boost_LIBRARIES} base config)

set_target_properties (
  remote PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_REMOTE_BUILD
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS remote
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)

if(APPLE)
  install(
    TARGETS remote
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/icinga-studio.app/Contents
  )
endif()

#install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/log\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/repository\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/zones\")")


