## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcib.la

libcib_la_SOURCES = \
	checkresult.cpp \
	checkresult.h \
	checktask.cpp \
	checktask.h \
	cib.cpp \
	cib.h \
	configobjectadapter.cpp \
	configobjectadapter.h \
	host.cpp \
	host.h \
	hostgroup.cpp \
	hostgroup.h \
	i2-cib.h \
	macroprocessor.cpp \
	macroprocessor.h \
	nagioschecktask.cpp \
	nagioschecktask.h \
	service.cpp \
	service.h \
	servicegroup.cpp \
	servicegroup.h

libcib_la_CPPFLAGS = \
	-DI2_CIB_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/third-party/popen-noshell

libcib_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libcib_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la \
	${top_builddir}/third-party/popen-noshell/libpopen_noshell.la
