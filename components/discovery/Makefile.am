## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =  \
	discovery.la

discovery_la_SOURCES =  \
	discoverycomponent.cpp \
	discoverycomponent.h \
	discoverymessage.cpp \
	discoverymessage.h \
	i2-discovery.h

discovery_la_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}/icinga

discovery_la_LDFLAGS = -module -version-info 0:0:0 -no-undefined -pthread
discovery_la_LIBADD = ${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/cJSON/libcJSON.la \
	${top_builddir}/icinga/libicinga.la
