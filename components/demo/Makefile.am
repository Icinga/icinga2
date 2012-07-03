## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	demo.la

demo_la_SOURCES = \
	democomponent.cpp \
	democomponent.h \
	i2-demo.h

demo_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga

demo_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

demo_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la
