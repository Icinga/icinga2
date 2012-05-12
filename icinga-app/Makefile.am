## Process this file with automake to produce Makefile.in


bin_PROGRAMS = \
	icinga

icinga_SOURCES = \
	icinga.cpp

icinga_CXXFLAGS = \
	-DI2_ICINGALAUNCHER_BUILD \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}

icinga_LDADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/icinga/libicinga.la \
	-dlopen ${top_builddir}/components/configfile/configfile.la \
	-dlopen ${top_builddir}/components/configrpc/configrpc.la \
	-dlopen ${top_builddir}/components/demo/demo.la \
	-dlopen ${top_builddir}/components/discovery/discovery.la

icinga_DEPENDENCIES = \
	${top_builddir}/components/configfile/configfile.la
