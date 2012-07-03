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
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/cib

discovery_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

discovery_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la \
	${top_builddir}/cib/libcib.la
