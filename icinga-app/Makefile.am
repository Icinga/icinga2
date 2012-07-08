## Process this file with automake to produce Makefile.in


bin_PROGRAMS = \
	icinga

icinga_SOURCES = \
	icinga.cpp

icinga_CPPFLAGS = \
	-DI2_ICINGALAUNCHER_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/dyn \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}

icinga_LDFLAGS = \
	$(BOOST_LDFLAGS)

icinga_LDADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/base/libbase.la \
	${top_builddir}/dyn/libdyn.la \
	${top_builddir}/icinga/libicinga.la \
	-dlopen ${top_builddir}/components/checker/checker.la \
	-dlopen ${top_builddir}/components/cibsync/cibsync.la \
	-dlopen ${top_builddir}/components/compat/compat.la \
	-dlopen ${top_builddir}/components/configfile/configfile.la \
	-dlopen ${top_builddir}/components/convenience/convenience.la \
	-dlopen ${top_builddir}/components/delegation/delegation.la \
	-dlopen ${top_builddir}/components/demo/demo.la \
	-dlopen ${top_builddir}/components/discovery/discovery.la

icinga_DEPENDENCIES = \
	${top_builddir}/components/configfile/configfile.la \
	${top_builddir}/components/convenience/convenience.la
