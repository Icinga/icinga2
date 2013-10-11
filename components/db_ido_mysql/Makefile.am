## Process this file with automake to produce Makefile.in

if MYSQL_USE

SUBDIRS = \
	schema

pkglib_LTLIBRARIES = \
	libdb_ido_mysql.la

CLEANFILES = \
	db_ido_mysql-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libdb_ido_mysql_la_SOURCES = \
	db_ido_mysql-type.conf \
	idomysqlconnection.cpp \
	idomysqlconnection.h

libdb_ido_mysql_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	$(MYSQL_CFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

libdb_ido_mysql_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(MYSQLR_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libdb_ido_mysql_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la \
	${top_builddir}/lib/db_ido/libdb_ido.la

else

all-local:
	@echo 'MySQL not enabled. Install libs/headers and rerun configure/make.'

endif
