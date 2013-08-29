## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libido.la

EXTRA_DIST = \
	ido-type.conf

BUILT_SOURCES = \
	ido-type.cpp

CLEANFILES = $(BUILT_SOURCES)

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libido_la_SOURCES = \
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
	ido-type.cpp \
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

libido_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib

libido_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libido_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la
