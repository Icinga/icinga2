## Process this file with automake to produce Makefile.in


noinst_LTLIBRARIES =  \
	libjsonrpc.la

libjsonrpc_la_SOURCES =  \
	i2-jsonrpc.h \
	jsonrpcclient.cpp \
	jsonrpcclient.h \
	jsonrpcmessage.cpp \
	jsonrpcmessage.h \
	jsonrpcserver.cpp \
	jsonrpcserver.h \
	netstring.cpp \
	netstring.h

libjsonrpc_la_CFLAGS = -DI2_JSONRPC_BUILD
libjsonrpc_la_CXXFLAGS = -DI2_JSONRPC_BUILD -I${top_srcdir}/base \
	-I${top_srcdir}/cJSON
