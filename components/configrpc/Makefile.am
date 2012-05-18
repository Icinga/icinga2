## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	configrpc.la

configrpc_la_SOURCES = \
	configrpccomponent.cpp \
	configrpccomponent.h \
	i2-configrpc.h

configrpc_la_CXXFLAGS = \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga

configrpc_la_LDFLAGS = \
	-module \
	-no-undefined \
	-version-info 0:0:0

configrpc_la_LIBADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la
