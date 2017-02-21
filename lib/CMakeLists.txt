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

add_subdirectory(base)
add_subdirectory(cli)
add_subdirectory(config)
add_subdirectory(remote)
add_subdirectory(icinga)
add_subdirectory(methods)

if(ICINGA2_WITH_HELLO)
  add_subdirectory(hello)
endif()

if(ICINGA2_WITH_CHECKER)
  add_subdirectory(checker)
endif()

if(ICINGA2_WITH_COMPAT)
  add_subdirectory(compat)
endif()

if(ICINGA2_WITH_MYSQL OR ICINGA2_WITH_PGSQL)
  add_subdirectory(db_ido)
endif()

if(ICINGA2_WITH_MYSQL)
  add_subdirectory(db_ido_mysql)
endif()

if(ICINGA2_WITH_PGSQL)
  add_subdirectory(db_ido_pgsql)
endif()

if(ICINGA2_WITH_DEMO)
  add_subdirectory(demo)
endif()

if(ICINGA2_WITH_LIVESTATUS)
  add_subdirectory(livestatus)
endif()

if(ICINGA2_WITH_NOTIFICATION)
  add_subdirectory(notification)
endif()

if(ICINGA2_WITH_PERFDATA)
  add_subdirectory(perfdata)
endif()

if(ICINGA2_WITH_REDIS)
  add_subdirectory(redis)
endif()

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}" PARENT_SCOPE)
