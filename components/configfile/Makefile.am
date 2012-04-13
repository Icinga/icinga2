## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES =  \
	configfile.la

configfile_la_SOURCES =  \
	configfile.cpp \
	configfile.h \
	i2-configfile.h

configfile_la_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON

configfile_la_LDFLAGS = -module -version-info 0:0:0 -no-undefined
configfile_la_LIBADD = ${top_builddir}/base/libbase.la \
	${top_builddir}/cJSON/libcJSON.la
