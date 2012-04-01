## Process this file with automake to produce Makefile.in


bin_PROGRAMS =  \
	miniapp

miniapp_SOURCES =  \
	miniapp.cpp

miniapp_CXXFLAGS = -I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc 

miniapp_LDFLAGS = $(top_builddir)/base/libbase.a \
	$(top_builddir)/jsonrpc/libjsonrpc.a -lltdl
