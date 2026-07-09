# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(BISON 2.3.0 REQUIRED)
find_package(FLEX 2.5.31 REQUIRED)

bison_target(class_parser class_parser.yy ${CMAKE_CURRENT_BINARY_DIR}/class_parser.cc)
flex_target(class_lexer class_lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/class_lexer.cc)
add_flex_bison_dependency(class_lexer class_parser)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/class_parser.cc ${CMAKE_CURRENT_BINARY_DIR}/class_lexer.cc
  PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION TRUE
)

if(NOT WIN32)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/class_parser.cc ${CMAKE_CURRENT_BINARY_DIR}/class_lexer.cc
    PROPERTIES
      COMPILE_FLAGS "-Wno-unused-but-set-variable"
  )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(mkclass_SOURCES
  mkclass.cpp
  classcompiler.cpp classcompiler.hpp
  ${FLEX_class_lexer_OUTPUTS}
  ${BISON_class_parser_OUTPUTS}
)

add_executable(mkclass ${mkclass_SOURCES})

if(WIN32)
  target_link_libraries(mkclass shlwapi)
endif()

set_target_properties (
  mkclass PROPERTIES
  FOLDER Bin
)

function(target_mkclass target generator_target)
  get_target_property(sources ${target} SOURCES)

  set(generated_headers)
  set(generated_impls)
  foreach(file ${sources})
    if(file MATCHES "\.ti$")
      get_filename_component(basename ${file} NAME_WE)
      set(generated_header ${CMAKE_CURRENT_BINARY_DIR}/${basename}-ti.hpp)
      set(generated_impl ${CMAKE_CURRENT_BINARY_DIR}/${basename}-ti.cpp)
      add_custom_command(
        OUTPUT ${generated_impl} ${generated_header}
        COMMAND mkclass
        ARGS ${file} ${generated_impl} ${generated_header}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS mkclass ${file}
      )
      list(APPEND generated_headers ${generated_header})
      list(APPEND generated_impls ${generated_impl})
    endif()
  endforeach()

  target_sources(${target} PRIVATE ${generated_headers})
  add_custom_target(${generator_target} DEPENDS ${generated_headers} ${generated_impls})
  add_dependencies(${target} ${generator_target})
endfunction()
