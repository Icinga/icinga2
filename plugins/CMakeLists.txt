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

if ( WIN32 )

  add_definitions ( -DUNICODE -D_UNICODE )
  
  add_library ( thresholds thresholds )
    set_target_properties (
    thresholds PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    FOLDER Plugins
  )
  
  list ( APPEND check_SOURCES 
    check_disk.cpp check_load.cpp check_memory.cpp check_network.cpp check_perfmon.cpp check_ping.cpp
    check_procs.cpp check_service.cpp check_swap.cpp check_update.cpp check_uptime.cpp check_users.cpp )
  
  foreach ( source ${check_SOURCES} ) 
    string ( REGEX REPLACE ".cpp\$" "" check_OUT "${source}" )
    string ( REGEX REPLACE ".cpp\$" ".h" check_HEADER "${source}" )
  
    add_executable ( ${check_OUT} ${source} ${check_HEADER} )
    target_link_libraries ( ${check_OUT} thresholds Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY} )
    
    set_target_properties (
      ${check_OUT} PROPERTIES
      INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
      DEFINE_SYMBOL I2_PLUGINS_BUILD
      FOLDER Plugins )
  endforeach ( source )

  target_link_libraries ( check_load Pdh.lib )
  target_link_libraries ( check_network Pdh.lib Iphlpapi.lib)
  target_link_libraries ( check_perfmon Pdh.lib )
  target_link_libraries ( check_ping Ntdll.lib iphlpapi.lib Ws2_32.lib )
  target_link_libraries ( check_procs Pdh.lib )
  target_link_libraries ( check_uptime ${Boost_SYSTEM_LIBRARY} )
  target_link_libraries ( check_users wtsapi32.lib )

  install (
    TARGETS check_disk check_load  check_memory check_network check_perfmon check_procs
      check_ping check_service check_swap check_update check_uptime check_users
      RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR} )

endif ( )
