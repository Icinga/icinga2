## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libreplication.la

EXTRA_DIST = \
	replication-type.conf

.conf.cpp: $(top_builddir)/tools/mkembedconfig.c
	$(top_builddir)/tools/mkembedconfig $< $@

libreplication_la_SOURCES = \
	replicationcomponent.cpp \
	replicationcomponent.h \
	replication-type.cpp

libreplication_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

libreplication_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libreplication_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/remoting/libremoting.la \
	${top_builddir}/lib/icinga/libicinga.la
