dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

m4_include([m4/ax_cxx_gcc_abi_demangle.m4])
m4_include([m4/ax_check_compile_flag.m4])
m4_include([m4/ax_pthread.m4])
m4_include([m4/ax_prog_doxygen.m4])

AC_INIT([icinga], [2.0])

AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])
AM_SILENT_RULES([yes])

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([icinga], [Doxyfile], [doc])

AC_PROG_CXX
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


AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[#include <openssl/ssl.h>]], []), [], [AC_MSG_ERROR([You need the OpenSSL headers and libraries in order to build this application]) ])


LT_INIT([dlopen, disable-static])
LT_CONFIG_LTDL_DIR([ltdl])
LTDL_INIT

if test "x$with_included_ltdl" != "xyes"; then
	AC_CHECK_LIB([ltdl], [lt_dladvise_init],
		[],
		[AC_MSG_ERROR([installed libltdl is too old])])
fi



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
