## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	configfile.la

configfile_la_SOURCES = \
	configfilecomponent.cpp \
	configfilecomponent.h \
	i2-configfile.h

configfile_la_CXXFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON

configfile_la_LDFLAGS = \
	-module \
	-no-undefined \
	-version-info @LIBRARY_VERSION@

configfile_la_LIBADD = \
	$(BOOST_LDFLAGS) \
	$(top_builddir)/base/libbase.la \
	$(top_builddir)/icinga/libicinga.la \
	$(top_builddir)/cJSON/libcJSON.la
