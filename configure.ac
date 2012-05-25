dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([icinga], [2.0])
AC_LANG(C++)

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])
AM_SILENT_RULES([yes])

LT_INIT([dlopen, disable-static])
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
AC_PROG_CXX
AC_PROG_LIBTOOL
AX_CXX_GCC_ABI_DEMANGLE
AX_PTHREAD

AC_CHECK_LIB(ssl, SSL_new)
AC_CHECK_LIB(crypto, X509_NAME_oneline)
AC_CHECK_LIB(eay32, X509_NAME_oneline)
AC_CHECK_LIB(m, floor)
AC_CHECK_LIB(pthread, pthread_create)

AX_CHECK_COMPILE_FLAG([-std=c++0x], [have_cxx11_support=yes])

if test "$have_cxx11_support" = "yes"; then
	CXXFLAGS="$CXXFLAGS -std=c++0x"
	AC_DEFINE(HAVE_CXX11, 1,
		[define if the compiler supports C++11])
fi


AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <openssl/ssl.h>]], [])], [], [AC_MSG_ERROR([You need the OpenSSL headers and libraries in order to build this application]) ])


AC_OUTPUT([
Makefile
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
Doxyfile
])
