# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(socketpair_SOURCES
  socketpair.c socketpair.h
)

add_library(socketpair OBJECT ${socketpair_SOURCES})

set_target_properties (
  socketpair PROPERTIES
  FOLDER Lib
)
