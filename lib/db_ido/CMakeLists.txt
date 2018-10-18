# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
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

mkclass_target(dbconnection.ti dbconnection-ti.cpp dbconnection-ti.hpp)

mkembedconfig_target(db_ido-itl.conf db_ido-itl.cpp)

set(db_ido_SOURCES
  i2-db_ido.hpp db_ido-itl.cpp
  commanddbobject.cpp commanddbobject.hpp
  dbconnection.cpp dbconnection.hpp dbconnection-ti.hpp
  dbevents.cpp dbevents.hpp
  dbobject.cpp dbobject.hpp
  dbquery.cpp dbquery.hpp
  dbreference.cpp dbreference.hpp
  dbtype.cpp dbtype.hpp
  dbvalue.cpp dbvalue.hpp
  endpointdbobject.cpp endpointdbobject.hpp
  hostdbobject.cpp hostdbobject.hpp
  hostgroupdbobject.cpp hostgroupdbobject.hpp
  idochecktask.cpp idochecktask.hpp
  servicedbobject.cpp servicedbobject.hpp
  servicegroupdbobject.cpp servicegroupdbobject.hpp
  timeperioddbobject.cpp timeperioddbobject.hpp
  userdbobject.cpp userdbobject.hpp
  usergroupdbobject.cpp usergroupdbobject.hpp
  zonedbobject.cpp zonedbobject.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(db_ido db_ido db_ido_SOURCES)
endif()

add_library(db_ido OBJECT ${db_ido_SOURCES})

add_dependencies(db_ido base config icinga remote)

set_target_properties (
  db_ido PROPERTIES
  FOLDER Lib
)
