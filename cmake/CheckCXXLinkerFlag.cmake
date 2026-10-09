# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(CheckCXXCompilerFlag)

function(check_cxx_linker_flag flag var)
  set(CMAKE_REQUIRED_FLAGS ${flag})
  set(result 0)
  check_cxx_compiler_flag(${flag} result)
  set(${var} ${result} PARENT_SCOPE)
endfunction()
