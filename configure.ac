/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

AC_INIT([icinga2], [0.0.3])
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])
AX_SET_VERSION_INFO
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_PROG_CC
AC_LANG_C
AC_PROG_MKDIR_P

AC_FUNC_VFORK

AC_ARG_WITH([icinga_user],
	    AC_HELP_STRING([--with-icinga-user=<user>],[sets user name to run icinga2]),
	    icinga_user=$withval,
	    icinga_user=icinga
)
ACICINGA_CHECK_USER([$icinga_user], run)
AC_SUBST(icinga_user)

AC_ARG_WITH(icinga_group,
	    AC_HELP_STRING([--with-icinga-group=<group>],[sets group name to run icinga2]),
	    icinga_group=$withval,
	    icinga_group=icinga
)
ACICINGA_CHECK_GROUP([$icinga_group], run)
AC_SUBST(icinga_group)

INSTALL_OPTS="-o $icinga_user -g $icinga_group"
AC_SUBST(INSTALL_OPTS)

AC_ARG_WITH(icingacmd_user,
	    AC_HELP_STRING([--with-icingacmd-user=<user>],[sets user name for icinga2 command access]),
	    icingacmd_user=$withval,
	    icingacmd_user=icinga
)
ACICINGA_CHECK_USER([$icingacmd_user], cmd)
AC_SUBST(icingacmd_user)

AC_ARG_WITH(icingacmd_group,
	    AC_HELP_STRING([--with-icingacmd-group=<group>] ,[sets group name for icinga2 command access]),
	    icingacmd_group=$withval,
	    icingacmd_group=icingacmd
)
ACICINGA_CHECK_GROUP([$icingacmd_group], cmd)
AC_SUBST(icingacmd_group)

COMMAND_OPTS="-o $icingacmd_user -g $icingacmd_group"
AC_SUBST(COMMAND_OPTS)


CFLAGS="$CFLAGS -D_GNU_SOURCE -pthread"
CXXFLAGS="$CXXFLAGS -D_GNU_SOURCE -pthread"

CFLAGS="$CFLAGS -I/opt/local/include"
CXXFLAGS="$CXXFLAGS -I/opt/local/include"
LDFLAGS="$LDFLAGS -L/opt/local/lib"

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
DX_INIT_DOXYGEN([icinga], [doc/Doxyfile], [doc/dev])

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
AC_CHECK_FUNCS([backtrace_symbols execvpe pipe2 pthread_set_name_np pthread_setname_np])

AC_TRY_LINK([#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/comp.h>], [BIO_f_zlib();], ac_cv_lib_biozlib="yes")
if test "$ac_cv_lib_biozlib" = "yes"; then
  AC_DEFINE(HAVE_BIOZLIB, 1,
    [Define if BIO_f_zlib is available])
fi

CFLAGS="$CFLAGS -Wall -Wextra"
CXXFLAGS="$CXXFLAGS -Wall -Wextra"

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

AC_MSG_CHECKING(whether to enable coverage)
AC_ARG_ENABLE(coverage, [ --enable-coverage=[no/yes]   turn on profiling (default=no)],, enable_coverage=no)
if test "x$enable_coverage" = "xyes"; then
	CFLAGS="$CFLAGS -g -O0 -DNDEBUG -fprofile-arcs -ftest-coverage -lgcov"
	CXXFLAGS="$CXXFLAGS -g -O0 -DNDEBUG -fprofile-arcs -ftest-coverage -lgcov"
fi
AC_MSG_RESULT($enable_coverage)

AX_LIB_MYSQL([5.0])
AC_CHECK_HEADERS([mysql/mysql.h], [mysql_use=true], [AC_MSG_WARN([mysql.h not found. Will not build mysql related libs/components.])])
AM_CONDITIONAL(MYSQL_USE, test x"$mysql_use" = x"true")

AX_PYTHON_DEFAULT
AX_PYTHON_ENABLE
AX_PYTHON_VERSION_ENSURE([2.5])
AX_PYTHON_CSPEC
AX_PYTHON_LSPEC

AM_COND_IF([PYTHON_USE], [AX_PYTHON_DEVEL([>= 2.5])])

AS_AC_EXPAND([ICINGA_PREFIX], $prefix)
AC_DEFINE_UNQUOTED([ICINGA_PREFIX], "$ICINGA_PREFIX", [The installation prefix.])

AS_AC_EXPAND([ICINGA_LOCALSTATEDIR], $localstatedir)
AC_DEFINE_UNQUOTED([ICINGA_LOCALSTATEDIR], "$ICINGA_LOCALSTATEDIR", [The local state dir.])

AS_AC_EXPAND([ICINGA_PKGLIBDIR], $libdir/$PACKAGE)
AC_DEFINE_UNQUOTED([ICINGA_PKGLIBDIR], "$ICINGA_PKGLIBDIR", [The package lib dir.])

AS_AC_EXPAND([ICINGA_PKGDATADIR], $datadir/$PACKAGE)
AC_DEFINE_UNQUOTED([ICINGA_PKGDATADIR], "$ICINGA_PKGDATADIR", [The package data dir.])

AC_CONFIG_FILES([
icinga2.spec
Makefile
components/Makefile
components/checker/Makefile
components/cluster/Makefile
components/compat/Makefile
components/demo/Makefile
components/db_ido_mysql/Makefile
components/db_ido_mysql/schema/Makefile
components/db_ido_mysql/schema/upgrade/Makefile
components/livestatus/Makefile
components/notification/Makefile
components/perfdata/Makefile
doc/Doxyfile
doc/Makefile
doc/strapdownjs/Makefile
doc/strapdownjs/themes/Makefile
etc/Makefile
etc/icinga/Makefile
etc/icinga2/Makefile
etc/icinga2/conf.d/Makefile
etc/icinga2/features-available/Makefile
etc/init.d/Makefile
icinga-app/Makefile
itl/Makefile
lib/Makefile
lib/base/Makefile
lib/config/Makefile
lib/icinga/Makefile
lib/db_ido/Makefile
lib/python/Makefile
pki/Makefile
pki/icinga2-build-ca
pki/icinga2-build-key
test/Makefile
third-party/Makefile
third-party/cJSON/Makefile
third-party/execvpe/Makefile
third-party/mmatch/Makefile
tools/Makefile
tools/migration/Makefile
tools/icinga2-enable-feature
tools/mkembedconfig/Makefile
])

AC_OUTPUT([
etc/init.d/icinga2
])

# Fix -rpath problem (https://wiki.debian.org/RpathIssue)
case ${host} in
  *-linux-gnu)
    sed 's|^sys_lib_dlsearch_path_spec=.*|sys_lib_dlsearch_path_spec="/lib64 /lib /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib/i686-linux-gnu /usr/local/lib64 /usr/local/lib"|g' libtool >libtool.new
    mv libtool.new libtool
  ;;
esac

if ! test -z "$LTDLDEPS"; then
	ltdl_msg="bundled"
else
	ltdl_msg="system-provided"
fi

AM_COND_IF([MYSQL_USE], [mysql_msg="enabled"], [mysql_msg="disabled"])
AM_COND_IF([PYTHON_USE], [python_msg="enabled"], [python_msg="disabled"])

echo "
Libraries:
     ltdl: $ltdl_msg

Features:
     MySQL: $mysql_msg
     Python: $python_msg

Users:
     User: $icinga_user
     Group: $icinga_group
     Command User: $icingacmd_user
     Command Group: $icingacmd_group

*** Please note that at this point Icinga 2 is highly experimental software
*** and is missing a significant portion of the planned features. This
*** is merely a technology preview and if you're planning to set up
*** a production environment for Icinga you should use Icinga 1.x instead.
"
