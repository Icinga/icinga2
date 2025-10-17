# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

link_directories(${PostgreSQL_LIBRARY_DIRS})
include_directories(SYSTEM ${PostgreSQL_INCLUDE_DIRS})

set(pgsql_shim_SOURCES
  pgsql_shim.def
  pgsqlinterface.cpp pgsqlinterface.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(pgsql_shim pgsql_shim pgsql_shim_SOURCES)
endif()

add_library(pgsql_shim SHARED ${pgsql_shim_SOURCES})

include(GenerateExportHeader)
generate_export_header(pgsql_shim)

target_link_libraries(pgsql_shim ${PostgreSQL_LIBRARIES})

set_target_properties (
  pgsql_shim PROPERTIES
  FOLDER Lib
  VERSION ${ICINGA2_PACKAGE_VERSION}
)

install(
  TARGETS pgsql_shim
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
)
