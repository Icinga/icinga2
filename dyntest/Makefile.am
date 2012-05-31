## Process this file with automake to produce Makefile.in


bin_PROGRAMS = \
	dyntest

dyntest_SOURCES = \
	dyntest.cpp

dyntest_CPPFLAGS = \
	-DI2_DYNTEST_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/dyn \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/components/configfile \
	-I${top_srcdir}

dyntest_LDFLAGS = \
	$(BOOST_LDFLAGS)

dyntest_LDADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/dyn/libdyn.la \
	${top_builddir}/components/configfile/configfile.la
