# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(mmatch)

add_subdirectory(socketpair)

add_subdirectory(nlohmann_json)

# TODO: Update vendored version because this one's CMakeLists.txt doesn't support old
# CMake versions anymore
# add_subdirectory(utf8cpp)
add_library(utf8cpp INTERFACE)
target_include_directories(utf8cpp INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/utf8cpp/source
)
