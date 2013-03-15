## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	liblivestatus.la

EXTRA_DIST = \
	livestatus-type.conf

.conf.cpp: $(top_builddir)/tools/mkembedconfig.c
	$(top_builddir)/tools/mkembedconfig $< $@

liblivestatus_la_SOURCES = \
	attributefilter.cpp \
	attributefilter.h \
	andfilter.cpp \
	andfilter.h \
	column.cpp \
	column.h \
	combinerfilter.cpp \
	combinerfilter.h \
	commentstable.cpp \
	commentstable.h \
	component.cpp \
	component.h \
	connection.cpp \
	connection.h \
	contactgroupstable.cpp \
	contactgroupstable.h \
	contactstable.cpp \
	contactstable.h \
	downtimestable.cpp \
	downtimestable.h \
	filter.cpp \
	filter.h \
	hoststable.cpp \
	hoststable.h \
	livestatus-type.cpp \
	negatefilter.cpp \
	negatefilter.h \
	orfilter.cpp \
	orfilter.h \
	query.cpp \
	query.h \
	servicestable.cpp \
	servicestable.h \
	statustable.cpp \
	statustable.h \
	table.cpp \
	table.h \
	i2-livestatus.h

liblivestatus_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/remoting \
	-I${top_srcdir}/lib/icinga

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
