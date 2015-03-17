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

# Tries to find termcap headers and libraries
#
# Usage of this module as follows:
#
#     find_package(Termcap)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  TERMCAP_ROOT_DIR  Set this variable to the root installation of
#                     termcap if the module has problems finding
#                     the proper installation path.
#
# Variables defined by this module:
#
#  TERMCAP_FOUND              System has Termcap libs/headers
#  TERMCAP_LIBRARIES          The Termcap libraries
#  TERMCAP_INCLUDE_DIR        The location of Termcap headers

find_path(TERMCAP_INCLUDE_DIR
  NAMES termcap.h
  HINTS ${TERMCAP_ROOT_DIR}/include)

find_library(TERMCAP_LIBRARIES
  NAMES termcap ncurses
  HINTS ${TERMCAP_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Termcap
  DEFAULT_MSG
  TERMCAP_LIBRARIES
  TERMCAP_INCLUDE_DIR)

mark_as_advanced(
  TERMCAP_ROOT_DIR
  TERMCAP_LIBRARIES
  TERMCAP_INCLUDE_DIR
  )
