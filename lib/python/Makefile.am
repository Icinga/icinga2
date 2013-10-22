## Process this file with automake to produce Makefile.in

if PYTHON_USE
pkglib_LTLIBRARIES =  \
	libpython.la

libpython_la_SOURCES =  \
	pythoninterpreter.cpp \
	pythoninterpreter.h \
	pythonlanguage.cpp \
	pythonlanguage.h

libpython_la_CPPFLAGS = \
	-DI2_PYTHON_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	@PYTHON_CSPEC@ \
	-I${top_srcdir}/lib

libpython_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	@PYTHON_LSPEC@
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libpython_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la
endif
