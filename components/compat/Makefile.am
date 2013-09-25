## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcompat.la

CLEANFILES = \
	compat-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libcompat_la_SOURCES = \
	checkresultreader.cpp \
	checkresultreader.h \
	externalcommandlistener.cpp \
	externalcommandlistener.h \
	statusdatawriter.cpp \
	statusdatawriter.h \
	compatlogger.cpp \
	compatlogger.h \
	compat-type.conf

libcompat_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

libcompat_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libcompat_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la
