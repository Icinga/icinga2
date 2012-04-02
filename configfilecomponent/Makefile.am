## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =  \
	libconfigfilecomponent.la

libconfigfilecomponent_la_SOURCES =  \
	configfilecomponent.cpp \
	configfilecomponent.h \
	i2-configfilecomponent.h

libconfigfilecomponent_la_CXXFLAGS = -I${top_srcdir}/base -I${top_srcdir}/jsonrpc

libconfigfilecomponent_la_LDFLAGS = -module -version-info 0:0:0 -no-undefined
libconfigfilecomponent_la_LIBADD = ${top_builddir}/base/libbase.la ${top_builddir}/jsonrpc/libjsonrpc.la