## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	cibsync.la

cibsync_la_SOURCES = \
	cibsynccomponent.cpp \
	cibsynccomponent.h \
	i2-cibsync.h

cibsync_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/jsonrpc \
	-I${top_srcdir}/lib/icinga \
	-I${top_srcdir}/lib/cib

cibsync_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

cibsync_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/jsonrpc/libjsonrpc.la \
	${top_builddir}/lib/icinga/libicinga.la \
	${top_builddir}/lib/cib/libcib.la
