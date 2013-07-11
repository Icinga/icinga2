## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	liblivestatus.la

EXTRA_DIST = \
	livestatus-type.conf

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig.c
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

liblivestatus_la_SOURCES = \
	attributefilter.cpp \
	attributefilter.h \
	andfilter.cpp \
	andfilter.h \
	column.cpp \
	column.h \
	combinerfilter.cpp \
	combinerfilter.h \
	commandstable.cpp \
	commandstable.h \
	commentstable.cpp \
	commentstable.h \
	component.cpp \
	component.h \
	contactgroupstable.cpp \
	contactgroupstable.h \
	contactstable.cpp \
	contactstable.h \
	downtimestable.cpp \
	downtimestable.h \
	filter.cpp \
	filter.h \
	hostgroupstable.cpp \
	hostgroupstable.h \
	hoststable.cpp \
	hoststable.h \
	livestatus-type.cpp \
	logtable.cpp \
	logtable.h \
	negatefilter.cpp \
	negatefilter.h \
	orfilter.cpp \
	orfilter.h \
	query.cpp \
	query.h \
	servicegroupstable.cpp \
	servicegroupstable.h \
	servicestable.cpp \
	servicestable.h \
	statustable.cpp \
	statustable.h \
	timeperiodstable.cpp \
	timeperiodstable.h \
	table.cpp \
	table.h

liblivestatus_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

liblivestatus_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

liblivestatus_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/remoting/libremoting.la \
	${top_builddir}/lib/icinga/libicinga.la
