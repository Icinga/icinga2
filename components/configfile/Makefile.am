## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	configfile.la

BUILD_SOURCES = icinga_parser.h

AM_YFLAGS = -d

configfile_la_SOURCES = \
	configcontext.cpp \
	configcontext.h \
	configfilecomponent.cpp \
	configfilecomponent.h \
	icinga_lexer.ll \
	icinga_parser.yy \
	i2-configfile.h

configfile_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/third-party/cJSON

configfile_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

configfile_la_LIBADD = \
	$(top_builddir)/base/libbase.la \
	$(top_builddir)/icinga/libicinga.la \
	$(top_builddir)/third-party/cJSON/libcJSON.la
