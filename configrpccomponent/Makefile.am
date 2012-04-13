## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =  \
	libconfigrpccomponent.la

libconfigrpccomponent_la_SOURCES =  \
	configrpccomponent.cpp \
	configrpccomponent.h \
	i2-configrpccomponent.h

libconfigrpccomponent_la_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}/icinga

libconfigrpccomponent_la_LDFLAGS = -module -version-info 0:0:0 -no-undefined -pthread
libconfigrpccomponent_la_LIBADD = ${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/cJSON/libcJSON.la \
	${top_builddir}/icinga/icinga
