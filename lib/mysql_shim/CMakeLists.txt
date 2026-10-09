# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(mysql_shim SHARED
  mysql_shim.def
  mysqlinterface.cpp mysqlinterface.hpp
)

target_link_libraries(mysql_shim PUBLIC
  MySQL
)

include(GenerateExportHeader)
generate_export_header(mysql_shim)

target_include_directories(mysql_shim PRIVATE
  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_BINARY_DIR}/lib
)

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
