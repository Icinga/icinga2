## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcompat.la

EXTRA_DIST = \
	compat-type.conf

.conf.cpp: $(top_builddir)/tools/mkembedconfig.c
	$(top_builddir)/tools/mkembedconfig $< $@

libcompat_la_SOURCES = \
	compatcomponent.cpp \
	compatcomponent.h \
	compat-type.cpp \
	i2-compat.h

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
	${top_builddir}/lib/remoting/libremoting.la \
	${top_builddir}/lib/icinga/libicinga.la
