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
	requestmessage.cpp \
	requestmessage.h \
	responsemessage.cpp \
	responsemessage.h

libjsonrpc_la_CPPFLAGS = \
	-DI2_JSONRPC_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base

libjsonrpc_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libjsonrpc_la_LIBADD = \
	${top_builddir}/lib/base/libbase.la
