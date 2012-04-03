dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(icinga, 2.0)

AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])

m4_include([m4/ax_cxx_gcc_abi_demangle.m4])

AM_SILENT_RULES([yes])

AC_PROG_CXX
AX_CXX_GCC_ABI_DEMANGLE





LT_INIT
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
configfilecomponent/Makefile
configrpccomponent/Makefile
icinga/Makefile
jsonrpc/Makefile

])
