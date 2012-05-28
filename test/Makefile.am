## Process this file with automake to produce Makefile.in

TESTS = \
	base-dictionary

check_PROGRAMS = \
	base-dictionary

base_dictionary_SOURCES = \
	base-dictionary.cpp

base_dictionary_CXXFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}

base_dictionary_LDADD = \
	$(BOOST_LDFLAGS) \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
	${top_builddir}/base/libbase.la
