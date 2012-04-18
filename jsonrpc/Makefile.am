## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libjsonrpc.la

libjsonrpc_la_SOURCES =  \
	i2-jsonrpc.h \
	jsonrpcclient.cpp \
	jsonrpcclient.h \
	jsonrpcrequest.cpp \
	jsonrpcrequest.h \
	jsonrpcresponse.cpp \
	jsonrpcresponse.h \
	jsonrpcserver.cpp \
	jsonrpcserver.h \
	message.cpp \
	message.h \
	netstring.cpp \
	netstring.h

libjsonrpc_la_CXXFLAGS = -DI2_JSONRPC_BUILD -I${top_srcdir}/base \
	-I${top_srcdir}/cJSON