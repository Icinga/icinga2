## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libdyn.la

BUILT_SOURCES = config_parser.h

AM_YFLAGS = -d

libdyn_la_SOURCES = \
	configcontext.cpp \
	configcontext.h \
	config_lexer.ll \
	config_parser.yy \
	i2-dyn.h \
	dconfigobject.cpp \
	dconfigobject.h \
	expression.cpp \
	expression.h \
	expressionlist.cpp \
	expressionlist.h \
	dynamicobject.cpp \
	dynamicobject.h \
	objectset.cpp \
	objectset.h \
	objectmap.cpp \
	objectmap.h

libdyn_la_CPPFLAGS = \
	-DI2_DYN_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base

libdyn_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libdyn_la_LIBADD = \
	${top_builddir}/base/libbase.la
