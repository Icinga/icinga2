## Process this file with automake to produce Makefile.in


bin_PROGRAMS =  \
	icinga

icinga_SOURCES =  \
	icinga.cpp

icinga_CFLAGS = -DI2_ICINGALAUNCHER_BUILD

icinga_CXXFLAGS = -DI2_ICINGALAUNCHER_BUILD -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}/libicinga \
	-I${top_srcdir}

icinga_LDFLAGS = $(top_builddir)/libicinga/libicinga.la \
	$(top_builddir)/jsonrpc/libjsonrpc.la \
	$(top_builddir)/base/libbase.la \
	${top_builddir}/cJSON/libcJSON.la
