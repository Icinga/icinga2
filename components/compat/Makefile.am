## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcompat.la

BUILT_SOURCES = \
	checkresultreader.th \
	externalcommandlistener.th \
	statusdatawriter.th \
	compatlogger.th

CLEANFILES = \
	compat-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

.ti.th: $(top_builddir)/tools/mkclass/mkclass
	$(top_builddir)/tools/mkclass/mkclass $< > $@

libcompat_la_SOURCES = \
	checkresultreader.cpp \
	checkresultreader.h \
	checkresultreader.th \
	externalcommandlistener.cpp \
	externalcommandlistener.h \
	externalcommandlistener.th \
	statusdatawriter.cpp \
	statusdatawriter.h \
	statusdatawriter.th \
	compatlogger.cpp \
	compatlogger.h \
	compatlogger.th \
	compat-type.conf

libcompat_la_CPPFLAGS = \
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
