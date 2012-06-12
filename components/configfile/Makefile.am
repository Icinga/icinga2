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
	-I${top_srcdir}/dyn

configfile_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

configfile_la_LIBADD = \
	$(top_builddir)/base/libbase.la \
	$(top_builddir)/dyn/libdyn.la
