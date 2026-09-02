# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_subdirectory(mmatch)

add_subdirectory(socketpair)

# icinga2 only links SQLiteCpp's (and its bundled sqlite3's) object files
# directly into the base library, so their own install(... COMPONENT
# headers/libraries) rules must never run: CPack registers a component name
# globally the moment install(COMPONENT) is called, regardless of whether
# that install rule ever actually executes, so even add_subdirectory(...
# EXCLUDE_FROM_ALL) (which does stop the files from being installed) still
# leaves "headers"/"libraries" registered. Their mere existence is enough to
# flip CPack's WiX generator into per-component symbol-prefixed File Ids,
# breaking the hardcoded, un-prefixed CM_FP_sbin.* references in
# icinga-installer/icinga2.wixpatch.cmake. Actually suppressing install()
# is the only thing that prevents the registration in the first place.
#
# A plain `function(install) endfunction()` override would work for
# SQLiteCpp itself, but a command override -- unlike a variable -- isn't
# undone when this directory's processing ends, so it would silently no-op
# every install() call for the rest of the project too (e.g.
# icinga-installer's). Gate it behind a flag instead, so the wrapper stays
# installed for the whole run but only actually suppresses while building
# SQLiteCpp's subtree.
set(icinga2_suppress_thirdparty_install ON)
function(install)
  if(NOT icinga2_suppress_thirdparty_install)
    _install(${ARGN})
  endif()
endfunction()
add_subdirectory(SQLiteCpp)
set(icinga2_suppress_thirdparty_install OFF)
