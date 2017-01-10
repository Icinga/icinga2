# Icinga 2
# Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.com)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

function(install_if_not_exists src dest)
  set(real_dest "${dest}")
  if(NOT IS_ABSOLUTE "${src}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
  endif()
  get_filename_component(src_name "${src}" NAME)
  get_filename_component(basename_dest "${src}" NAME)
  string(REPLACE "/" "\\\\" nsis_src "${src}")
  string(REPLACE "/" "\\\\" nsis_dest_dir "${real_dest}")
  string(REPLACE "/" "\\\\" nsis_dest "${real_dest}/${basename_dest}")
  install(CODE "
   if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
      set(target_dir \${CMAKE_INSTALL_PREFIX})
    else()
      set(target_dir \$ENV{DESTDIR})
    endif()
    if(\${CMAKE_INSTALL_PREFIX} MATCHES .*/_CPack_Packages/.* OR NOT EXISTS \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/\${skel_prefix}${dest}/${src_name}\")
      message(STATUS \"Installing: \$ENV{DESTDIR}${dest}/${src_name}\")
      if(\${CMAKE_INSTALL_PREFIX} MATCHES .*/_CPack_Packages/.*)
        set(skel_prefix \"share/skel/\")
      else()
        set(skel_prefix \"\")
      endif()
      execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${src}\"
                      \"\${target_dir}/\${skel_prefix}${dest}/${src_name}\"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR \${error_output})
      endif()
    else()
      message(STATUS \"Skipping  : \${target_dir}/${dest}/${src_name}\")
    endif()
  ")
endfunction(install_if_not_exists)
