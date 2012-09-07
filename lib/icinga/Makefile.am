## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libicinga.la

libicinga_la_SOURCES =  \
	endpoint.cpp \
	endpoint.h \
	endpointmanager.cpp \
	endpointmanager.h \
	icingaapplication.cpp \
	icingaapplication.h \
	i2-icinga.h

libicinga_la_CPPFLAGS = \
	-DI2_ICINGA_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/jsonrpc \
	-I${top_srcdir}

libicinga_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libicinga_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/jsonrpc/libjsonrpc.la
