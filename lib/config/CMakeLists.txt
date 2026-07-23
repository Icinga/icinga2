# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(BISON 2.3.0 REQUIRED)
find_package(FLEX 2.5.31 REQUIRED)

bison_target(config_parser config_parser.yy ${CMAKE_CURRENT_BINARY_DIR}/config_parser.cc)
flex_target(config_lexer config_lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.cc)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/config_parser.cc ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.cc
  PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION TRUE
)

add_flex_bison_dependency(config_lexer config_parser)

add_library(config OBJECT
  i2-config.hpp
  activationcontext.cpp activationcontext.hpp
  applyrule.cpp applyrule-targeted.cpp applyrule.hpp
  configcompiler.cpp configcompiler.hpp
  configcompilercontext.cpp configcompilercontext.hpp
  activationcontext.hpp
  applyrule.hpp
  configcompiler.hpp
  configcompilercontext.hpp
  configfragment.hpp
  configitem.cpp configitem.hpp
  configitembuilder.cpp configitembuilder.hpp
  expression.cpp expression.hpp
  objectrule.cpp objectrule.hpp
  configitem.hpp
  configitembuilder.hpp
  expression.hpp
  objectrule.hpp
  vmops.hpp
  ${FLEX_config_lexer_OUTPUTS} ${BISON_config_parser_OUTPUTS}
)

target_link_libraries(config PUBLIC
  common
)

add_dependencies(config base_generated)

set_target_properties (
  config PROPERTIES
  FOLDER Lib
)
