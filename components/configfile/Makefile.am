## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	configfile.la

configfile_la_SOURCES = \
	configfilecomponent.cpp \
	configfilecomponent.h \
	i2-configfile.h

configfile_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/third-party/cJSON

configfile_la_LDFLAGS = \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

configfile_la_LIBADD = \
	$(BOOST_LDFLAGS) \
	$(top_builddir)/base/libbase.la \
	$(top_builddir)/icinga/libicinga.la \
	$(top_builddir)/third-party/cJSON/libcJSON.la
