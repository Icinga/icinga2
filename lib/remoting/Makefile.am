## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libremoting.la

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig.c
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libremoting_la_SOURCES = \
	endpoint.cpp \
	endpoint.h \
	endpointmanager.cpp \
	endpointmanager.h \
	i2-remoting.h \
	jsonrpc.cpp \
	jsonrpc.h \
	remoting-type.cpp

libremoting_la_CPPFLAGS = \
	-DI2_REMOTING_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib

libremoting_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libremoting_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la
