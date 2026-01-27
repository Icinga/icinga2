# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

if(MSVC)
  include_external_msproject(
    icinga2setupagent
    ${CMAKE_CURRENT_SOURCE_DIR}/windows-setup-agent/Icinga2SetupAgent.csproj
    TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC
  )

  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/windows-setup-agent/bin/\${CMAKE_INSTALL_CONFIG_NAME}/Icinga2SetupAgent.exe ${CMAKE_CURRENT_SOURCE_DIR}/windows-setup-agent/bin/\${CMAKE_INSTALL_CONFIG_NAME}/Icinga2SetupAgent.exe.config
    DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )
endif()
