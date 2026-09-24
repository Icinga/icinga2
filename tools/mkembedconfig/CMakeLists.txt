# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(mkembedconfig_SOURCES
  mkembedconfig.c
)

add_executable(mkembedconfig ${mkembedconfig_SOURCES})

set_target_properties (
  mkembedconfig PROPERTIES
  FOLDER Bin
)

macro(MKEMBEDCONFIG_TARGET EmbedInput EmbedOutput)
  set(_codegen)
  if(POLICY CMP0171)
    set(_codegen CODEGEN)
  endif()

  add_custom_command(
    OUTPUT ${EmbedOutput}
    COMMAND mkembedconfig
    ARGS ${EmbedInput} ${CMAKE_CURRENT_BINARY_DIR}/${EmbedOutput}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS mkembedconfig ${EmbedInput}
    ${_codegen}
  )
  set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${EmbedOutput} PROPERTY SKIP_UNITY_BUILD_INCLUSION TRUE)
endmacro()

