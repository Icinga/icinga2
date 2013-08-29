## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcluster.la

EXTRA_DIST = \
	cluster-type.conf

BUILT_SOURCES = \
	cluster-type.cpp

CLEANFILES = $(BUILT_SOURCES)

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libcluster_la_SOURCES = \
	clustercomponent.cpp \
	clustercomponent.h \
	cluster-type.cpp \
	endpoint.cpp \
	endpoint.h \
	jsonrpc.cpp \
	jsonrpc.h

libcluster_la_CPPFLAGS = \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

libcluster_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libcluster_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la
