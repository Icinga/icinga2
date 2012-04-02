## Process this file with automake to produce Makefile.in


noinst_LTLIBRARIES =  \
	libjsonrpc.la

libjsonrpc_la_SOURCES =  \
	cJSON.c \
	cJSON.h \
	connectionmanager.cpp \
	connectionmanager.h \
	i2-jsonrpc.h \
	jsonrpcclient.cpp \
	jsonrpcclient.h \
	jsonrpcmessage.cpp \
	jsonrpcmessage.h \
	jsonrpcserver.cpp \
	jsonrpcserver.h \
	netstring.cpp \
	netstring.h

libjsonrpc_la_CXXFLAGS = -I${top_srcdir}/base 
