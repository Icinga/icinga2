# Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+

foreach(flag_var
  CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
  CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if(${flag_var} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

set(icinga_installer_SOURCES
  icinga-installer.cpp
)

add_executable(icinga-installer ${icinga_installer_SOURCES})

set_target_properties(
  icinga-installer PROPERTIES
  FOLDER Bin
  OUTPUT_NAME icinga2-installer
  LINK_FLAGS "/SUBSYSTEM:WINDOWS"

  # Use a statically-linked runtime library as this binary is run during the installation process where the other DLLs
  # may not have been installed already and the system-provided version may be too old.
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_link_libraries(icinga-installer shlwapi)

install(CODE "
  execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${CMAKE_CURRENT_BINARY_DIR}/icinga2.wixpatch.\${BUILD_TYPE}\"
    \"${CMAKE_CURRENT_BINARY_DIR}/icinga2.wixpatch\"
    RESULT_VARIABLE copy_result
    ERROR_VARIABLE error_output)
  if(copy_result)
    message(FATAL_ERROR \${error_output})
  endif()"
)

file(
  GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/icinga2.wixpatch.$<CONFIG>"
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/icinga2.wixpatch.cmake"
)

set(InstallPath "${CMAKE_INSTALL_SBINDIR}")

install(
  TARGETS icinga-installer
  RUNTIME DESTINATION ${InstallPath}
)
