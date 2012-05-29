## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	configrpc.la

configrpc_la_SOURCES = \
	configrpccomponent.cpp \
	configrpccomponent.h \
	i2-configrpc.h

configrpc_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga

configrpc_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

configrpc_la_LIBADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la
