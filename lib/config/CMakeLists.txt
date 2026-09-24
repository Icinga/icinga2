# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(BISON 2.3.0 REQUIRED)
find_package(FLEX 2.5.31 REQUIRED)

set(CONFIG_PARSER_CC ${CMAKE_CURRENT_BINARY_DIR}/config_parser.cc)
set(CONFIG_PARSER_HH ${CMAKE_CURRENT_BINARY_DIR}/config_parser.hh)
set(CONFIG_LEXER_CC ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.cc)

add_custom_command(
  OUTPUT ${CONFIG_PARSER_CC} ${CONFIG_PARSER_HH}
  COMMAND ${BISON_EXECUTABLE} --defines=${CONFIG_PARSER_HH} -o ${CONFIG_PARSER_CC}
          ${CMAKE_CURRENT_SOURCE_DIR}/config_parser.yy
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS config_parser.yy
  COMMENT "[BISON][config_parser] Building parser with bison ${BISON_VERSION}"
  VERBATIM
  ${ICINGA2_CODEGEN}
)

add_custom_command(
  OUTPUT ${CONFIG_LEXER_CC}
  COMMAND ${FLEX_EXECUTABLE} -o${CONFIG_LEXER_CC}
          ${CMAKE_CURRENT_SOURCE_DIR}/config_lexer.ll
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS config_lexer.ll ${CONFIG_PARSER_HH}
  COMMENT "[FLEX][config_lexer] Building scanner with flex ${FLEX_VERSION}"
  VERBATIM
  ${ICINGA2_CODEGEN}
)

set_source_files_properties(${CONFIG_PARSER_CC} ${CONFIG_LEXER_CC}
  PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION TRUE
)

add_custom_target(config_parser
  DEPENDS ${CONFIG_PARSER_CC} ${CONFIG_PARSER_HH} ${CONFIG_LEXER_CC}
)

add_library(config OBJECT
  i2-config.hpp
  activationcontext.cpp activationcontext.hpp
  applyrule.cpp applyrule-targeted.cpp applyrule.hpp
  configcompiler.cpp configcompiler.hpp
  configcompilercontext.cpp configcompilercontext.hpp
  configfragment.hpp
  configitem.cpp configitem.hpp
  configitembuilder.cpp configitembuilder.hpp
  expression.cpp expression.hpp
  objectrule.cpp objectrule.hpp
  vmops.hpp
  ${CONFIG_PARSER_CC} ${CONFIG_LEXER_CC}
)

target_link_libraries(config PUBLIC
  common
)

add_dependencies(config base_generated)

set_target_properties (
  config PROPERTIES
  FOLDER Lib
)
