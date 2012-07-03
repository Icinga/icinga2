## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	cibsync.la

cibsync_la_SOURCES = \
	cibsynccomponent.cpp \
	cibsynccomponent.h \
	i2-cibsync.h

cibsync_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/cib

cibsync_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

cibsync_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la \
	${top_builddir}/cib/libcib.la
