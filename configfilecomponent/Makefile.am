## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =  \
	configfilecomponent.la

configfilecomponent_la_SOURCES =  \
	configfilecomponent.cpp \
	configfilecomponent.h \
	i2-configfilecomponent.h

configfilecomponent_la_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON

configfilecomponent_la_LDFLAGS = -module -version-info 0:0:0 -no-undefined
configfilecomponent_la_LIBADD = ${top_builddir}/base/libbase.la \
	${top_builddir}/cJSON/libcJSON.la
