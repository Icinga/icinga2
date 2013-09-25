## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libdb_ido.la

CLEANFILES = \
	db_ido-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libdb_ido_la_SOURCES = \
	commanddbobject.cpp \
	commanddbobject.h \
	dbconnection.cpp \
	dbconnection.h \
	dbobject.cpp \
	dbobject.h \
	dbquery.cpp \
	dbquery.h \
	dbreference.cpp \
	dbreference.h \
	dbtype.cpp \
	dbtype.h \
	dbvalue.cpp \
	dbvalue.h \
	hostdbobject.cpp \
	hostdbobject.h \
	hostgroupdbobject.cpp \
	hostgroupdbobject.h \
	db_ido-type.conf \
	servicedbobject.cpp \
	servicedbobject.h \
	servicegroupdbobject.cpp \
	servicegroupdbobject.h \
	timeperioddbobject.cpp \
	timeperioddbobject.h \
	userdbobject.cpp \
	userdbobject.h \
	usergroupdbobject.cpp \
	usergroupdbobject.h

libdb_ido_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib

libdb_ido_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libdb_ido_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la
