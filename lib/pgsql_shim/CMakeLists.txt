# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(pgsql_shim SHARED
  pgsql_shim.def
  pgsqlinterface.cpp pgsqlinterface.hpp
)

target_link_libraries(pgsql_shim PUBLIC
  PostgreSQL
)

include(GenerateExportHeader)
generate_export_header(pgsql_shim)

target_include_directories(pgsql_shim PRIVATE
  ${CMAKE_SOURCE_DIR}/lib
  ${CMAKE_BINARY_DIR}/lib
)

set_target_properties (
  pgsql_shim PROPERTIES
  FOLDER Lib
  VERSION ${SPEC_VERSION}
)

install(
  TARGETS pgsql_shim
  RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
)
