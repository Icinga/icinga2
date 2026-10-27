# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(mmatch STATIC
  mmatch.c mmatch.h
)

target_include_directories(mmatch
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT MSVC)
  target_compile_options(mmatch
    PRIVATE
    -Wno-implicit-fallthrough
  )
endif()

set_target_properties(
  mmatch PROPERTIES
  FOLDER Lib
)
