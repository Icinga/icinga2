## Process this file with automake to produce Makefile.in

TESTS = \
	icinga2_test

check_PROGRAMS = \
	icinga2_test

icinga2_test_SOURCES = \
	test.cpp \
	base-dictionary.cpp

icinga2_test_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/icinga \
	-I${top_srcdir}

icinga2_test_LDADD = \
	$(BOOST_LDFLAGS) \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/icinga/libicinga.la
