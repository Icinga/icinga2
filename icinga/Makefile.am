## Process this file with automake to produce Makefile.in


bin_PROGRAMS =  \
	icinga

icinga_SOURCES =  \
	icingaapplication.cpp \
	icingaapplication.h \
	i2-icinga.h

icinga_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc  -I${top_srcdir}

icinga_LDFLAGS = $(top_builddir)/jsonrpc/libjsonrpc.la \
	$(top_builddir)/base/libbase.la