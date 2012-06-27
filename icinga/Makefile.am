## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libicinga.la

libicinga_la_SOURCES =  \
	checkresult.cpp \
	checkresult.h \
	checktask.cpp \
	checktask.h \
	cib.cpp \
	cib.h \
	configobjectadapter.cpp \
	configobjectadapter.h \
	endpoint.cpp \
	endpoint.h \
	endpointmanager.cpp \
	endpointmanager.h \
	icingaapplication.cpp \
	icingaapplication.h \
	host.cpp \
	host.h \
	i2-icinga.h \
	jsonrpcendpoint.cpp \
	jsonrpcendpoint.h \
	macroprocessor.cpp \
	macroprocessor.h \
	nagioschecktask.cpp \
	nagioschecktask.h \
	service.cpp \
	service.h \
	virtualendpoint.cpp \
	virtualendpoint.h

libicinga_la_CPPFLAGS = \
	-DI2_ICINGA_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/cJSON \
	-I${top_srcdir}/third-party/popen-noshell \
	-I${top_srcdir}

libicinga_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libicinga_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/base/libbase.la \
	${top_builddir}/third-party/popen-noshell/libpopen_noshell.la
