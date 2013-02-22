## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libremoting.la

libremoting_la_SOURCES = \
	endpoint.cpp \
	endpoint.h \
	endpointmanager.cpp \
	endpointmanager.h \
	i2-remoting.cpp \
	i2-remoting.h \
	jsonrpcconnection.cpp \
	jsonrpcconnection.h \
	messagepart.cpp \
	messagepart.h \
	requestmessage.cpp \
	requestmessage.h \
	responsemessage.cpp \
	responsemessage.h

libremoting_la_CPPFLAGS = \
	-DI2_REMOTING_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config

libremoting_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libremoting_la_LIBADD = \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la
