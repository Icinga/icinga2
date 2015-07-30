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

mkclass_target(apilistener.ti apilistener.tcpp apilistener.thpp)
mkclass_target(apiuser.ti apiuser.tcpp apiuser.thpp)
mkclass_target(endpoint.ti endpoint.tcpp endpoint.thpp)
mkclass_target(zone.ti zone.tcpp zone.thpp)

set(remote_SOURCES
  actionshandler.cpp apiaction.cpp
  apifunction.cpp apilistener.cpp apilistener.thpp apilistener-sync.cpp
  apiuser.cpp apiuser.thpp authority.cpp base64.cpp configfileshandler.cpp
  configmoduleshandler.cpp configmoduleutility.cpp configstageshandler.cpp
  endpoint.cpp endpoint.thpp filterutility.cpp
  httpchunkedencoding.cpp  httpconnection.cpp httphandler.cpp httprequest.cpp httpresponse.cpp
  httputility.cpp jsonrpc.cpp jsonrpcconnection.cpp jsonrpcconnection-heartbeat.cpp
  messageorigin.cpp statusqueryhandler.cpp url.cpp zone.cpp zone.thpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(remote remote_SOURCES)
endif()

add_library(remote SHARED ${remote_SOURCES})

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(remote ${Boost_LIBRARIES} base config)

set_target_properties (
  remote PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_REMOTE_BUILD
  FOLDER Lib
)

install(
  TARGETS remote
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)

#install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/log\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/repository\")")
install(CODE "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/icinga2/api/zones\")")


