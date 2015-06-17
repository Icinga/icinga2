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

mkclass_target(dbconnection.ti dbconnection.tcpp dbconnection.thpp)

mkembedconfig_target(db_ido-itl.conf db_ido-itl.cpp)

set(db_ido_SOURCES
  commanddbobject.cpp dbconnection.cpp dbconnection.thpp
  db_ido-itl.cpp dbevents.cpp dbobject.cpp dbquery.cpp
  dbreference.cpp dbtype.cpp dbvalue.cpp endpointdbobject.cpp hostdbobject.cpp
  hostgroupdbobject.cpp idochecktask.cpp servicedbobject.cpp
  servicegroupdbobject.cpp timeperioddbobject.cpp userdbobject.cpp
  usergroupdbobject.cpp zonedbobject.cpp
)

if(ICINGA2_UNITY_BUILD)
    mkunity_target(db_ido db_ido_SOURCES)
endif()

add_library(db_ido SHARED ${db_ido_SOURCES})

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(db_ido ${Boost_LIBRARIES} base config icinga remote)

set_target_properties (
  db_ido PROPERTIES
  INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
  DEFINE_SYMBOL I2_DB_IDO_BUILD
  FOLDER Lib
)

install(
  TARGETS db_ido
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/icinga2
)
