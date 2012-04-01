## Process this file with automake to produce Makefile.in


bin_PROGRAMS =  \
	icinga

icinga_SOURCES =  \
	icingaapplication.cpp \
	icingaapplication.h

icinga_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc 

icinga_LDFLAGS = $(top_builddir)/base/libbase.a \
	$(top_builddir)/jsonrpc/libjsonrpc.a 
