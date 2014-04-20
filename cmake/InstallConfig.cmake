function(install_if_not_exists src dest)
  set(real_dest "${dest}")
  if(NOT IS_ABSOLUTE "${src}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
  endif()
  get_filename_component(src_name "${src}" NAME)
  if (NOT IS_ABSOLUTE "${dest}")
    set(dest "${CMAKE_INSTALL_PREFIX}/${dest}")
  endif()
  get_filename_component(basename_dest "${src}" NAME)
  string(REPLACE "/" "\\\\" nsis_src "${src}")
  string(REPLACE "/" "\\\\" nsis_dest_dir "${real_dest}")
  string(REPLACE "/" "\\\\" nsis_dest "${real_dest}/${basename_dest}")
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
    SetOverwrite off
    CreateDirectory '$INSTDIR\\\\${nsis_dest_dir}'
    File '/oname=${nsis_dest}' '${nsis_src}'
    SetOverwrite on
  " PARENT_SCOPE)
  install(CODE "
    if(NOT EXISTS \"\$ENV{DESTDIR}${dest}/${src_name}\")
      #file(INSTALL \"${src}\" DESTINATION \"${dest}\")
      message(STATUS \"Installing: \$ENV{DESTDIR}${dest}/${src_name}\")
      execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${src}\"
                      \"\$ENV{DESTDIR}${dest}/${src_name}\"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR \${error_output})
      endif()
    else()
      message(STATUS \"Skipping  : \$ENV{DESTDIR}${dest}/${src_name}\")
    endif()
  ")
endfunction(install_if_not_exists)
