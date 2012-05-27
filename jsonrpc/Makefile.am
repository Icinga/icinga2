## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libjsonrpc.la

libjsonrpc_la_SOURCES = \
	i2-jsonrpc.h \
	jsonrpcclient.cpp \
	jsonrpcclient.h \
	jsonrpcserver.cpp \
	jsonrpcserver.h \
	messagepart.cpp \
	messagepart.h \
	netstring.cpp \
	netstring.h \
	requestmessage.cpp \
	requestmessage.h \
	responsemessage.cpp \
	responsemessage.h

libjsonrpc_la_CXXFLAGS = \
	-DI2_JSONRPC_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/cJSON

libjsonrpc_la_LDFLAGS = \
	-no-undefined \
	-version-info @LIBRARY_VERSION@

libjsonrpc_la_LIBADD = \
	$(BOOST_LDFLAGS) \
	${top_builddir}/base/libbase.la \
	${top_builddir}/cJSON/libcJSON.la
