# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(socketpair OBJECT
  socketpair.c socketpair.h
)

target_include_directories(socketpair
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(socketpair INTERFACE $<TARGET_OBJECTS:socketpair>)

set_target_properties (
  socketpair PROPERTIES
  FOLDER Lib
)
