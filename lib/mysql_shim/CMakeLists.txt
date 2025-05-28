# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

include_directories(SYSTEM ${MYSQL_INCLUDE_DIR})

set(mysql_shim_SOURCES
  mysql_shim.def
  mysqlinterface.cpp mysqlinterface.hpp
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(mysql_shim mysql_shim mysql_shim_SOURCES)
endif()

add_library(mysql_shim SHARED ${mysql_shim_SOURCES})

include(GenerateExportHeader)
generate_export_header(mysql_shim)

target_link_libraries(mysql_shim ${MYSQL_LIB})

set_target_properties (
  mysql_shim PROPERTIES
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS mysql_shim
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
)
