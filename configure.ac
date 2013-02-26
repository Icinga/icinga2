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
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_PROG_CC
AC_LANG_C

AC_FUNC_VFORK

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
DX_INIT_DOXYGEN([icinga], [docs/Doxyfile], [docs/dev])
AD_INIT_ASCIIDOC([icinga], [docs])

AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)

AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL
AX_CXX_GCC_ABI_DEMANGLE
AX_BOOST_BASE([1.41], [], [AC_MSG_ERROR([You need the Boost headers and libraries in order to build this application])])
AX_BOOST_SIGNALS
AX_BOOST_THREAD
AX_BOOST_SYSTEM
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_BOOST_PROGRAM_OPTIONS
AX_CHECK_OPENSSL([], [AC_MSG_ERROR([You need the OpenSSL headers and libraries in order to build this application])])
AC_CHECK_LIB(ssl, SSL_new)
AC_CHECK_LIB(crypto, X509_NAME_oneline)
AC_CHECK_LIB(eay32, X509_NAME_oneline)
AC_CHECK_LIB(m, floor)
AC_CHECK_LIB(socket, getsockname)
AC_CHECK_LIB(ws2_32, getsockname)
AC_CHECK_LIB(shlwapi, PathRemoveFileSpecA)
AC_CHECK_FUNCS([backtrace_symbols execvpe pipe2])

AC_MSG_CHECKING(whether to enable debugging)
AC_ARG_ENABLE(debug, [  --enable-debug=[no/yes]   turn on debugging (default=no)],, enable_debug=no)
if test "x$enable_debug" = "xyes"; then
	CFLAGS="$CFLAGS -g -O0 -D_DEBUG"
	CXXFLAGS="$CXXFLAGS -g -O0 -D_DEBUG"
else
	CFLAGS="$CFLAGS -DNDEBUG"
	CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi
AC_MSG_RESULT($enable_debug)

AX_C___ATTRIBUTE__
if test "$ax_cv___attribute__" = "yes" && test "x$enable_debug" = "xno"; then
	CFLAGS="$CFLAGS -fvisibility=hidden"
	CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
fi

AX_PYTHON_DEFAULT
AX_PYTHON_ENABLE
AX_PYTHON_VERSION_ENSURE([2.5])
AX_PYTHON_CSPEC
AX_PYTHON_LSPEC

AS_AC_EXPAND([ICINGA_PREFIX], $prefix)
AC_DEFINE_UNQUOTED([ICINGA_PREFIX], "$ICINGA_PREFIX", [The installation prefix.])

AS_AC_EXPAND([ICINGA_LOCALSTATEDIR], $localstatedir)
AC_DEFINE_UNQUOTED([ICINGA_LOCALSTATEDIR], "$ICINGA_LOCALSTATEDIR", [The local state dir.])

AS_AC_EXPAND([ICINGA_PKGLIBDIR], $libdir/$PACKAGE)
AC_DEFINE_UNQUOTED([ICINGA_PKGLIBDIR], "$ICINGA_PKGLIBDIR", [The package lib dir.])

AS_AC_EXPAND([ICINGA_PKGDATADIR], $datadir/$PACKAGE)
AC_DEFINE_UNQUOTED([ICINGA_PKGDATADIR], "$ICINGA_PKGDATADIR", [The package data dir.])

AC_CONFIG_FILES([
Makefile
components/Makefile
components/checker/Makefile
components/compat/Makefile
components/compatido/Makefile
components/delegation/Makefile
components/demo/Makefile
components/notification/Makefile
components/replication/Makefile
docs/Doxyfile
docs/Makefile
etc/Makefile
etc/icinga2/Makefile
etc/init.d/Makefile
icinga-app/Makefile
itl/Makefile
lib/Makefile
lib/base/Makefile
lib/config/Makefile
lib/icinga/Makefile
lib/python/Makefile
lib/remoting/Makefile
test/Makefile
third-party/Makefile
third-party/cJSON/Makefile
third-party/execvpe/Makefile
third-party/mmatch/Makefile
third-party/popen-noshell/Makefile
])
AC_OUTPUT([
etc/init.d/icinga2
])

if ! test -z "$LTDLDEPS"; then
	ltdl_msg="bundled"
else
	ltdl_msg="system-provided"
fi

echo "
Libraries:
     ltdl: $ltdl_msg

*** Please note that at this point Icinga 2 is highly experimental software
*** and is missing a significant portion of the planned features. This
*** is merely a technology preview and if you're planning to set up
*** a production environment for Icinga you should use Icinga 1.x instead.
"
