## Process this file with automake to produce Makefile.in

TESTS = \
	icinga2_test

check_PROGRAMS = \
	icinga2_test

icinga2_test_SOURCES = \
	test.cpp \
	base-array.cpp \
	base-convert.cpp \
	base-dictionary.cpp \
	base-fifo.cpp \
	base-match.cpp \
	base-netstring.cpp \
	base-object.cpp \
	base-shellescape.cpp \
	base-stacktrace.cpp \
	base-string.cpp \
	base-timer.cpp \
	base-value.cpp

icinga2_test_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib

icinga2_test_LDADD = \
	$(BOOST_LDFLAGS) \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/icinga/libicinga.la
