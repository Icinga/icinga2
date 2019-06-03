# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

add_executable(check_nscp_api
  check_nscp_api.cpp
  ${base_OBJS}
  $<TARGET_OBJECTS:config>
  $<TARGET_OBJECTS:remote>
)
target_link_libraries(check_nscp_api ${base_DEPS})
set_target_properties (
  check_nscp_api PROPERTIES
  DEFINE_SYMBOL I2_PLUGINS_BUILD
  FOLDER Plugins)

# Prefer the PluginDir constant which is set to /sbin on Windows

if(WIN32)
  install(TARGETS check_nscp_api RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
else()
  install(TARGETS check_nscp_api RUNTIME DESTINATION ${ICINGA2_PLUGINDIR})
endif()

if (WIN32)
  add_definitions(-DUNICODE -D_UNICODE)

  set(thresholds_SOURCES
    thresholds.cpp thresholds.hpp
  )

  add_library(thresholds ${thresholds_SOURCES})

  set_target_properties(
    thresholds PROPERTIES
    FOLDER Plugins
  )

  set(check_SOURCES
    check_disk.cpp check_load.cpp check_memory.cpp check_network.cpp check_perfmon.cpp
    check_ping.cpp check_procs.cpp check_service.cpp check_swap.cpp check_update.cpp check_uptime.cpp
    check_users.cpp
  )

  foreach(source ${check_SOURCES})
    string(REGEX REPLACE ".cpp\$" "" check_OUT "${source}")

    add_executable(${check_OUT} ${source})
    target_link_libraries(${check_OUT} thresholds shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})

    set_target_properties(
      ${check_OUT} PROPERTIES
      DEFINE_SYMBOL I2_PLUGINS_BUILD
      FOLDER Plugins
    )
  endforeach()

  target_link_libraries(check_load pdh.lib)
  target_link_libraries(check_network pdh.lib iphlpapi.lib)
  target_link_libraries(check_perfmon pdh.lib)
  target_link_libraries(check_ping ntdll.lib iphlpapi.lib ws2_32.lib)
  target_link_libraries(check_procs pdh.lib)
  target_link_libraries(check_uptime ${Boost_SYSTEM_LIBRARY})
  target_link_libraries(check_users wtsapi32.lib)

  install(
    TARGETS check_disk check_load check_memory check_network check_perfmon check_procs
      check_ping check_service check_swap check_update check_uptime check_users
    RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )
endif ( )
