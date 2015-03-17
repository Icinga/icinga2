# Copyright (c) 2014, Matthias Vallentin
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     1. Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
# 
#     2. Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
# 
#     3. Neither the name of the copyright holder nor the names of its
#        contributors may be used to endorse or promote products derived from
#        this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Tries to find editline headers and libraries
#
# Usage of this module as follows:
#
#     find_package(Editline)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  EDITLINE_ROOT_DIR  Set this variable to the root installation of
#                     editline if the module has problems finding
#                     the proper installation path.
#
# Variables defined by this module:
#
#  EDITLINE_FOUND              System has Editline libs/headers
#  EDITLINE_LIBRARIES          The Editline libraries
#  EDITLINE_INCLUDE_DIR        The location of Editline headers
#  EDITLINE_VERSION            The full version of Editline
#  EDITLINE_VERSION_MAJOR      The version major of Editline
#  EDITLINE_VERSION_MINOR      The version minor of Editline

find_path(EDITLINE_INCLUDE_DIR
  NAMES histedit.h
  HINTS ${EDITLINE_ROOT_DIR}/include)

if (EDITLINE_INCLUDE_DIR)
  file(STRINGS ${EDITLINE_INCLUDE_DIR}/histedit.h editline_header REGEX "^#define.LIBEDIT_[A-Z]+.*$")

  string(REGEX REPLACE ".*#define.LIBEDIT_MAJOR[ \t]+([0-9]+).*" "\\1" EDITLINE_VERSION_MAJOR "${editline_header}")
  string(REGEX REPLACE ".*#define.LIBEDIT_MINOR[ \t]+([0-9]+).*" "\\1" EDITLINE_VERSION_MINOR "${editline_header}")

  set(EDITLINE_VERSION_MAJOR ${EDITLINE_VERSION_MAJOR} CACHE STRING "" FORCE)
  set(EDITLINE_VERSION_MINOR ${EDITLINE_VERSION_MINOR} CACHE STRING "" FORCE)
  set(EDITLINE_VERSION ${EDITLINE_VERSION_MAJOR}.${EDITLINE_VERSION_MINOR}
    CACHE STRING "" FORCE)
endif ()

find_library(EDITLINE_LIBRARIES
  NAMES edit
  HINTS ${EDITLINE_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Editline
  DEFAULT_MSG
  EDITLINE_LIBRARIES
  EDITLINE_INCLUDE_DIR)

mark_as_advanced(
  EDITLINE_ROOT_DIR
  EDITLINE_LIBRARIES
  EDITLINE_INCLUDE_DIR
  EDITLINE_VERSION
  EDITLINE_VERSION_MAJOR
  EDITLINE_VERSION_MINOR
  )
