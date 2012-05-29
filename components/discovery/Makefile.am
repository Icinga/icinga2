## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =  \
	discovery.la

discovery_la_SOURCES =  \
	discoverycomponent.cpp \
	discoverycomponent.h \
	discoverymessage.cpp \
	discoverymessage.h \
	i2-discovery.h

discovery_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga

discovery_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

discovery_la_LIBADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la
