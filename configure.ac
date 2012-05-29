/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

AC_INIT
AX_SPEC_FILE([icinga2.spec])
AX_SPEC_DEFAULTS
AX_SET_VERSION_INFO
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
#AM_SILENT_RULES([yes])

AC_CONFIG_SUBDIRS([third-party/sqlite])

AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_PROG_CC
AC_LANG_C

LT_INIT([dlopen disable-static win32-dll])
LT_CONFIG_LTDL_DIR([third-party/ltdl])
LTDL_INIT

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([icinga], [Doxyfile], [doc])

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AX_CXX_COMPILE_STDCXX_0X
AX_CXX_GCC_ABI_DEMANGLE
AX_PTHREAD
AX_BOOST_BASE
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_CHECK_OPENSSL([], [AC_MSG_ERROR([You need the OpenSSL headers and libraries in order to build this application])])
AC_CHECK_LIB(ssl, SSL_new)
AC_CHECK_LIB(crypto, X509_NAME_oneline)
AC_CHECK_LIB(eay32, X509_NAME_oneline)
AC_CHECK_LIB(m, floor)
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(socket, getsockname)
AC_CHECK_LIB(ws2_32, getsockname)
AC_CHECK_LIB(shlwapi, PathRemoveFileSpecA)

AC_CHECK_LIB(sqlite3, sqlite3_open)

AC_MSG_CHECKING([for SQLite3])
AC_LANG_PUSH(C)
AC_COMPILE_IFELSE(
	AC_LANG_PROGRAM([[#include <sqlite3.h>]], []),
	[
	AC_MSG_RESULT([yes])
	bundled_sqlite3=no
	],
	[
	AC_MSG_ERROR([no])
	bundled_sqlite3=yes
	]
	)
AC_LANG_POP(C)

AC_ARG_ENABLE(bundled-sqlite3,
	[  --disable-bundled-sqlite3
                          disables building the bundled sqlite3 library],
	[bundled_sqlite3=$enableval])
AM_CONDITIONAL([USE_BUNDLED_SQLITE3], [test "$bundled_sqlite3" = yes])

if test "$bundled_sqlite3" = yes; then
	SQLITE3_LDFLAGS=${top_builddir}/third-party/sqlite/libsqlite3.la
	SQLITE3_CPPFLAGS=-I${top_srcdir}/third-party/sqlite
else
	SQLITE3_LDFLAGS=
	SQLITE3_CPPFLAGS=
fi
AC_SUBST([SQLITE3_LDFLAGS])
AC_SUBST([SQLITE3_CPPFLAGS])

AC_CONFIG_FILES([
Makefile
compat/Makefile
base/Makefile
components/Makefile
components/configfile/Makefile
components/configrpc/Makefile
components/demo/Makefile
components/discovery/Makefile
icinga/Makefile
icinga-app/Makefile
jsonrpc/Makefile
test/Makefile
third-party/Makefile
third-party/cJSON/Makefile
third-party/mmatch/Makefile
Doxyfile
])
AC_OUTPUT

if test "$bundled_sqlite3" = yes; then
	sqlite3_msg="bundled"
else
	sqlite3_msg="system-provided"
fi

if ! test -z "$LTDLDEPS"; then
	ltdl_msg="bundled"
else
	ltdl_msg="system-provided"
fi

echo "
Libraries:
     sqlite3: $sqlite3_msg
     ltdl: $ltdl_msg
"
