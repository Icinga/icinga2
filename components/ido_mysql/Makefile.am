## Process this file with automake to produce Makefile.in

if MYSQL_USE
pkglib_LTLIBRARIES = \
	libido_mysql.la

EXTRA_DIST = \
	ido_mysql-type.conf

BUILT_SOURCES = \
	ido_mysql-type.cpp

CLEANFILES = $(BUILT_SOURCES)

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libido_mysql_la_SOURCES = \
	ido_mysql-type.cpp \
	idomysqldbconnection.cpp \
	idomysqldbconnection.h

libido_mysql_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	$(MYSQL_CFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

libido_mysql_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(MYSQLR_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libido_mysql_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la \
	${top_builddir}/lib/ido/libido.la

else

all-local:
	@echo 'MySQL not enabled. Install libs/headers and rerun configure/make.'

endif
