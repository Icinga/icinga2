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
	icingacomponent.cpp \
	icingacomponent.h \
	i2-icinga.h \
	jsonrpcendpoint.cpp \
	jsonrpcendpoint.h \
	virtualendpoint.cpp \
	virtualendpoint.h

libicinga_la_CPPFLAGS = \
	-DI2_ICINGA_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}

libicinga_la_LDFLAGS = \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libicinga_la_LIBADD = \
	$(BOOST_LDFLAGS) \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/base/libbase.la
