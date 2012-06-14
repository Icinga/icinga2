## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	checker.la

checker_la_SOURCES = \
	checkercomponent.cpp \
	checkercomponent.h \
	i2-checker.h

checker_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga

checker_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

checker_la_LIBADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/icinga/libicinga.la
