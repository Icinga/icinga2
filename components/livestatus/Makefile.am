## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	livestatus.la

livestatus_la_SOURCES = \
	andfilter.cpp \
	andfilter.h \
	combinerfilter.cpp \
	combinerfilter.h \
	component.cpp \
	component.h \
	connection.cpp \
	connection.h \
	filter.cpp \
	filter.h \
	negatefilter.cpp \
	negastefilter.h \
	orfilter.cpp \
	orfilter.h \
	query.cpp \
	query.h \
	statustable.cpp \
	statustable.h \
	table.cpp \
	table.h \
	i2-livestatus.h

livestatus_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/remoting \
	-I${top_srcdir}/lib/icinga

livestatus_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

livestatus_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/remoting/libremoting.la \
	${top_builddir}/lib/icinga/libicinga.la
