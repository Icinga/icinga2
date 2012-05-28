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
AM_SILENT_RULES([yes])

AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_PROG_CC
AC_LANG_C

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AX_CXX_COMPILE_STDCXX_0X
AX_CXX_GCC_ABI_DEMANGLE
AX_PTHREAD

LT_INIT([dlopen shared disable-static win32-dll])
LT_CONFIG_LTDL_DIR([ltdl])
LTDL_INIT

AX_BOOST_BASE
AX_BOOST_UNIT_TEST_FRAMEWORK

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
AC_PROG_CC
AC_PROG_LIBTOOL
AX_CXX_COMPILE_STDCXX_0X
AX_CXX_GCC_ABI_DEMANGLE
AX_PTHREAD

AC_CHECK_LIB(ssl, SSL_new)
AC_CHECK_LIB(crypto, X509_NAME_oneline)
AC_CHECK_LIB(eay32, X509_NAME_oneline)
AC_CHECK_LIB(m, floor)
AC_CHECK_LIB(pthread, pthread_create)
AC_CHECK_LIB(socket, getsockname)
AC_CHECK_LIB(ws2_32, getsockname)
AC_CHECK_LIB(shlwapi, PathRemoveFileSpecA)

AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <openssl/ssl.h>]], [])],
	[],
	[AC_MSG_ERROR([You need the OpenSSL headers and libraries in order to build this application]) ])

AC_OUTPUT([
Makefile
compat/Makefile
base/Makefile
cJSON/Makefile
components/Makefile
components/configfile/Makefile
components/configrpc/Makefile
components/demo/Makefile
components/discovery/Makefile
icinga/Makefile
icinga-app/Makefile
jsonrpc/Makefile
mmatch/Makefile
test/Makefile
Doxyfile
])
