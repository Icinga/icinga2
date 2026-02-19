# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(mmatch)

if(UNIX OR CYGWIN)
  add_subdirectory(execvpe)
endif()

add_subdirectory(socketpair)

add_subdirectory(SQLiteCpp)
