## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libdyn.la

libdyn_la_SOURCES = \
	i2-dyn.h \
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
