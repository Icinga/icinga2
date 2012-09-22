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

AC_PROG_INSTALL
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL
AX_CXX_GCC_ABI_DEMANGLE
AX_BOOST_BASE([1.46], [], [AC_MSG_ERROR([You need the Boost headers and libraries in order to build this application])])
AX_BOOST_SIGNALS
AX_BOOST_THREAD
AX_BOOST_SYSTEM
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_CHECK_OPENSSL([], [AC_MSG_ERROR([You need the OpenSSL headers and libraries in order to build this application])])
AC_CHECK_LIB(ssl, SSL_new)
AC_CHECK_LIB(crypto, X509_NAME_oneline)
AC_CHECK_LIB(eay32, X509_NAME_oneline)
AC_CHECK_LIB(m, floor)
AC_CHECK_LIB(socket, getsockname)
AC_CHECK_LIB(ws2_32, getsockname)
AC_CHECK_LIB(shlwapi, PathRemoveFileSpecA)

AC_CONFIG_FILES([
Makefile
components/Makefile
components/checker/Makefile
components/compat/Makefile
components/compatido/Makefile
components/convenience/Makefile
components/delegation/Makefile
components/demo/Makefile
components/replication/Makefile
docs/Doxyfile
icinga-app/Makefile
icinga-app/config/Makefile
lib/Makefile
lib/base/Makefile
lib/config/Makefile
lib/icinga/Makefile
lib/remoting/Makefile
test/Makefile
third-party/Makefile
third-party/cJSON/Makefile
third-party/mmatch/Makefile
third-party/popen-noshell/Makefile
])
AC_OUTPUT

if ! test -z "$LTDLDEPS"; then
	ltdl_msg="bundled"
else
	ltdl_msg="system-provided"
fi

echo "
Libraries:
     ltdl: $ltdl_msg
"
