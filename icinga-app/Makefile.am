## Process this file with automake to produce Makefile.in

bin_PROGRAMS = \
	icinga

icinga_SOURCES = \
	icinga.cpp

icinga_CPPFLAGS = \
	-DI2_ICINGALAUNCHER_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/remoting \
	-I${top_srcdir}/lib/icinga \
	-I${top_srcdir}

icinga_LDFLAGS = \
	$(BOOST_LDFLAGS)

icinga_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/remoting/libremoting.la \
	${top_builddir}/lib/icinga/libicinga.la \
	-dlopen ${top_builddir}/components/checker/checker.la \
	-dlopen ${top_builddir}/components/replication/replication.la \
	-dlopen ${top_builddir}/components/compat/compat.la \
	-dlopen ${top_builddir}/components/convenience/convenience.la \
	-dlopen ${top_builddir}/components/delegation/delegation.la \
	-dlopen ${top_builddir}/components/demo/demo.la

icinga_DEPENDENCIES = \
	${top_builddir}/components/replication/replication.la \
	${top_builddir}/components/convenience/convenience.la
