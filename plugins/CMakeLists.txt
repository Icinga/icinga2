if(WIN32)
  add_definitions(-DUNICODE -D_UNICODE)

  add_library(thresholds thresholds)

  add_executable(check_disk check_disk.cpp)
  target_link_libraries(check_disk thresholds Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_disk PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )

  add_executable(check_load check_load.cpp)
  target_link_libraries(check_load thresholds Pdh.lib Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_load PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_network check_network.cpp)
  target_link_libraries(check_network thresholds Pdh.lib Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_network PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_procs check_procs.cpp)
  target_link_libraries(check_procs thresholds Pdh.lib Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_procs PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_service check_service.cpp)
  target_link_libraries(check_service thresholds Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_service PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_swap check_swap.cpp)
  target_link_libraries(check_swap thresholds Pdh.lib Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_swap PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_update check_update.cpp)
  target_link_libraries(check_update thresholds Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_update PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_uptime check_uptime.cpp)
  target_link_libraries(check_uptime thresholds ${Boost_SYSTEM_LIBRARY} Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
  set_target_properties (
    check_uptime PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
  add_executable(check_users check_users.cpp)
  target_link_libraries(check_users thresholds wtsapi32.lib Shlwapi.lib ${Boost_PROGRAM_OPTIONS_LIBRARY})
	  set_target_properties (
    check_users PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/icinga2
    DEFINE_SYMBOL I2_PLUGINS_BUILD
    FOLDER Plugins
  )
	
	install(
    TARGETS check_disk check_load check_network check_procs check_service check_swap check_update check_uptime check_users
    RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )	
endif()