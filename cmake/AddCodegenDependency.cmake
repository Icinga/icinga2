# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Enable the use of the CODEGEN keyword in add_custom_command(). This is supported with CMake 3.31
# onwards and isn't needed for packaging, but useful for running all the codegen steps without
# building the source, for example to enable clang-tidy linting in the GitHub CI.
if(POLICY CMP0171)
  cmake_policy(SET CMP0171 NEW)
else()
  # Since this CMake version doesn't know about the policy, the `codegen` target isn't reserved
  # and we can add it as a fallback to the canonical one.
  add_custom_target(codegen)
endif()

function(add_codegen_dependency target)
  string(RANDOM LENGTH 16 _name)

  if(POLICY CMP0171)
    set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${_name}.codegen.stamp")

    add_custom_command(
      OUTPUT "${_stamp}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
      DEPENDS ${ARGN}
      CODEGEN
    )

    target_sources(${target} PRIVATE "${_stamp}")
  else()
    add_custom_target(${_name} DEPENDS ${ARGN})
    add_dependencies(codegen ${_name})
    add_dependencies(${target} ${_name})
  endif()
endfunction()
