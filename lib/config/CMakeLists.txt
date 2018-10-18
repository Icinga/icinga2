# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

find_package(BISON 2.3.0 REQUIRED)
find_package(FLEX 2.5.31 REQUIRED)

bison_target(config_parser config_parser.yy ${CMAKE_CURRENT_BINARY_DIR}/config_parser.cc)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/config_parser.cc PROPERTY EXCLUDE_UNITY_BUILD TRUE)

flex_target(config_lexer config_lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.cc)
set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.cc PROPERTY EXCLUDE_UNITY_BUILD TRUE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/config_parser.cc PROPERTY COMPILE_FLAGS "-Wno-deprecated-register -Wno-parentheses-equality -Wno-unused-function")
  set_property(SOURCE ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.cc PROPERTY COMPILE_FLAGS "-Wno-deprecated-register -Wno-null-conversion")
endif()

add_flex_bison_dependency(config_lexer config_parser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(config_SOURCES
  i2-config.hpp
  activationcontext.cpp activationcontext.hpp
  applyrule.cpp applyrule.hpp
  configcompiler.cpp configcompiler.hpp
  configcompilercontext.cpp configcompilercontext.hpp
  configfragment.hpp
  configitem.cpp configitem.hpp
  configitembuilder.cpp configitembuilder.hpp
  expression.cpp expression.hpp
  objectrule.cpp objectrule.hpp
  vmops.hpp
  ${FLEX_config_lexer_OUTPUTS} ${BISON_config_parser_OUTPUTS}
)

if(ICINGA2_UNITY_BUILD)
  mkunity_target(config config config_SOURCES)
endif()

add_library(config OBJECT ${config_SOURCES})

add_dependencies(config base)

set_target_properties (
  config PROPERTIES
  FOLDER Lib
)
