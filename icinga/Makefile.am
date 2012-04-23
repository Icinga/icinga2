## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libicinga.la

libicinga_la_SOURCES =  \
	authenticationcomponent.cpp \
	authenticationcomponent.h \
	discoverycomponent.cpp \
	discoverycomponent.h \
	endpoint.cpp \
	endpoint.h \
	endpointmanager.cpp \
	endpointmanager.h \
	icingaapplication.cpp \
	icingaapplication.h \
	identitymessage.cpp \
	identitymessage.h \
	i2-icinga.h \
	jsonrpcendpoint.cpp \
	jsonrpcendpoint.h \
	subscriptioncomponent.cpp \
	subscriptioncomponent.h \
	subscriptionmessage.cpp \
	subscriptionmessage.h \
	virtualendpoint.cpp \
	virtualendpoint.h

libicinga_la_CFLAGS = -DI2_ICINGA_BUILD

libicinga_la_CXXFLAGS = -DI2_ICINGA_BUILD -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}

libicinga_la_LDFLAGS = $(top_builddir)/jsonrpc/libjsonrpc.la \
	$(top_builddir)/base/libbase.la \
	${top_builddir}/cJSON/libcJSON.la
