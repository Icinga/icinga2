# Copyright (c) 2007-2014, Lloyd Hilaiel <me@lloyd.io>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

SET (SRCS yajl.c yajl_lex.c yajl_parser.c yajl_buf.c
          yajl_encode.c yajl_gen.c yajl_alloc.c
          yajl_tree.c yajl_version.c
)
SET (HDRS yajl_parser.h yajl_lex.h yajl_buf.h yajl_encode.h yajl_alloc.h)
SET (PUB_HDRS api/yajl_parse.h api/yajl_gen.h api/yajl_common.h api/yajl_tree.h)

# useful when fixing lexer bugs.
#ADD_DEFINITIONS(-DYAJL_LEXER_DEBUG)

# Ensure defined when building YAJL (as opposed to using it from
# another project).  Used to ensure correct function export when
# building win32 DLL.
ADD_DEFINITIONS(-DYAJL_BUILD)

# set up some paths
SET (incDir ${CMAKE_CURRENT_BINARY_DIR}/../include/yajl)

ADD_LIBRARY(yajl OBJECT ${SRCS} ${HDRS} ${PUB_HDRS})

#### setup shared library version number
SET_TARGET_PROPERTIES(yajl
    PROPERTIES
    FOLDER Lib
)

#### build up an sdk as a post build step

# create some directories
FILE(MAKE_DIRECTORY ${incDir})

# generate build-time source
CONFIGURE_FILE(api/yajl_version.h.cmake ${incDir}/yajl_version.h)

# copy public headers to output directory
FOREACH (header ${PUB_HDRS})
  SET (header "${CMAKE_CURRENT_SOURCE_DIR}/${header}")

  EXEC_PROGRAM(${CMAKE_COMMAND} ARGS -E copy_if_different \"${header}\" \"${incDir}\")
ENDFOREACH (header ${PUB_HDRS})

INCLUDE_DIRECTORIES(${incDir}/..)
