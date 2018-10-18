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


if(NOT WIN32)
  configure_file(icinga2.sysconfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/initsystem/icinga2.sysconfig @ONLY)
  get_filename_component(ICINGA2_SYSCONFIGFILE_NAME ${ICINGA2_SYSCONFIGFILE} NAME)
  get_filename_component(ICINGA2_SYSCONFIGFILE_DIR ${ICINGA2_SYSCONFIGFILE} PATH)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/initsystem/icinga2.sysconfig
    DESTINATION ${ICINGA2_SYSCONFIGFILE_DIR}
    RENAME ${ICINGA2_SYSCONFIGFILE_NAME}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  )

  configure_file(prepare-dirs.cmake ${CMAKE_CURRENT_BINARY_DIR}/initsystem/prepare-dirs @ONLY)
  configure_file(safe-reload.cmake ${CMAKE_CURRENT_BINARY_DIR}/initsystem/safe-reload @ONLY)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/initsystem/prepare-dirs ${CMAKE_CURRENT_BINARY_DIR}/initsystem/safe-reload
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/icinga2
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

  # required for packaging on Gentoo, see Bug #6498
  option (INSTALL_SYSTEMD_SERVICE_AND_INITSCRIPT
    "Force install both the systemd service definition file and the SysV initscript in parallel, regardless of how USE_SYSTEMD is set. Only use this for special packaging purposes and if you know what you are doing" OFF)

  if (NOT USE_SYSTEMD OR INSTALL_SYSTEMD_SERVICE_AND_INITSCRIPT)
	  configure_file(icinga2.init.d.cmake ${CMAKE_CURRENT_BINARY_DIR}/initsystem/icinga2 @ONLY)
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/initsystem/icinga2
      DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
  endif()

  if (USE_SYSTEMD OR INSTALL_SYSTEMD_SERVICE_AND_INITSCRIPT)
    configure_file(icinga2.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/initsystem/icinga2.service @ONLY)
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/initsystem/icinga2.service
      DESTINATION ${DESTDIR}/usr/lib/systemd/system
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
  endif()
endif()
