## Process this file with automake to produce Makefile.in


bin_PROGRAMS =  \
	icinga

icinga_SOURCES =  \
	endpoint.cpp \
	endpoint.h \
	endpointmanager.cpp \
	endpointmanager.h \
	icingaapplication.cpp \
	icingaapplication.h \
	i2-icinga.h \
	jsonrpcendpoint.cpp \
	jsonrpcendpoint.h \
	virtualendpoint.cpp \
	virtualendpoint.h

icinga_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}

icinga_LDFLAGS = $(top_builddir)/jsonrpc/libjsonrpc.la \
	$(top_builddir)/base/libbase.la \
	${top_builddir}/cJSON/libcJSON.la