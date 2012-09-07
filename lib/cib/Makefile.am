## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcib.la

libcib_la_SOURCES = \
	cib.cpp \
	cib.h \
	host.cpp \
	host.h \
	hostgroup.cpp \
	hostgroup.h \
	i2-cib.h \
	macroprocessor.cpp \
	macroprocessor.h \
	nagioschecktask.cpp \
	nagioschecktask.h \
	nullchecktask.cpp \
	nullchecktask.h \
	service.cpp \
	service.h \
	servicegroup.cpp \
	servicegroup.h \
	servicestatechangemessage.cpp \
	servicestatechangemessage.h

libcib_la_CPPFLAGS = \
	-DI2_CIB_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/jsonrpc \
	-I${top_srcdir}/lib/icinga

libcib_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libcib_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/jsonrpc/libjsonrpc.la \
	${top_builddir}/lib/icinga/libicinga.la
