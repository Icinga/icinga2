# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

mkembedconfig_target(db_ido-itl.conf db_ido-itl.cpp)

set(db_ido_SOURCES
  i2-db_ido.hpp db_ido-itl.cpp
  commanddbobject.cpp commanddbobject.hpp
  dbconnection.cpp dbconnection.hpp dbconnection.ti
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

target_mkclass(db_ido db_ido_generated)
add_dependencies(db_ido base_generated icinga_generated remote_generated)

set_target_properties (
  db_ido PROPERTIES
  FOLDER Lib
)
