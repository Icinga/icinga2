## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = \
	icinga2

icinga2_SOURCES = \
	icinga.cpp

icinga2_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

icinga2_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-export-dynamic

icinga2_LDADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la

