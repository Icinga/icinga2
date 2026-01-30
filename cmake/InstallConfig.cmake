# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Install $src into directory $dest - usually only used for config files
#
# * similar to install() a non absolute path is prefixed with CMAKE_INSTALL_PREFIX on runtime
# * in case of CPack path with be prefixed with share/skel/
# * DESTDIR is prefixed as well
#
# also see https://cmake.org/cmake/help/latest/command/install.html

function(install_if_not_exists src dest)
  if(NOT IS_ABSOLUTE "${src}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
  endif()

  get_filename_component(src_name "${src}" NAME)

  install(CODE "
    set(dest \"${dest}\")

    if (\"\${CMAKE_INSTALL_PREFIX}\" MATCHES .*/_CPack_Packages/.*)
      set(dest \"share/skel/\${dest}\")
      set(force_overwrite TRUE)
    else()
      set(force_overwrite FALSE)
    endif()

    if(NOT IS_ABSOLUTE \"\${dest}\")
      set(dest \"\${CMAKE_INSTALL_PREFIX}/\${dest}\")
    endif()

    set(full_dest \"\$ENV{DESTDIR}\${dest}/${src_name}\")

    if(force_overwrite OR NOT EXISTS \"\${full_dest}\")
      message(STATUS \"Installing: ${src} into \${full_dest}\")

      execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${src}\" \"\${full_dest}\"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR \${error_output})
      endif()
    else()
      message(STATUS \"Skipping  : \${full_dest}\")
    endif()
  ")
endfunction(install_if_not_exists)
