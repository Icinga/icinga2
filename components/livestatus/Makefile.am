## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	liblivestatus.la

CLEANFILES = \
	livestatus-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

liblivestatus_la_SOURCES = \
	aggregator.cpp \
	aggregator.h \
	attributefilter.cpp \
	attributefilter.h \
	andfilter.cpp \
	andfilter.h \
	avgaggregator.cpp \
	avgaggregator.h \
	column.cpp \
	column.h \
	combinerfilter.cpp \
	combinerfilter.h \
	commandstable.cpp \
	commandstable.h \
	commentstable.cpp \
	commentstable.h \
	contactgroupstable.cpp \
	contactgroupstable.h \
	contactstable.cpp \
	contactstable.h \
	countaggregator.cpp \
	countaggregator.h \
	downtimestable.cpp \
	downtimestable.h \
	filter.cpp \
	filter.h \
	hostgroupstable.cpp \
	hostgroupstable.h \
	hoststable.cpp \
	hoststable.h \
	invavgaggregator.cpp \
	invavgaggregator.h \
	invsumaggregator.cpp \
	invsumaggregator.h \
	listener.cpp \
	listener.h \
	livestatus-type.conf \
	logtable.cpp \
	logtable.h \
	maxaggregator.cpp \
	maxaggregator.h \
	minaggregator.cpp \
	minaggregator.h \
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
	stdaggregator.cpp \
	stdaggregator.h \
	sumaggregator.cpp \
	sumaggregator.h \
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
	${top_builddir}/lib/icinga/libicinga.la
