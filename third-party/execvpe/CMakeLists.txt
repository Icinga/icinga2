# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(execvpe_SOURCES
  execvpe.c execvpe.h
)

add_library(execvpe OBJECT ${execvpe_SOURCES})

set_target_properties (
  execvpe PROPERTIES
  FOLDER Lib
)
