## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libcluster.la

BUILT_SOURCES = \
	clusterlistener.th \
	endpoint.th

nodist_libcluster_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES = \
	cluster-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

.ti.th: $(top_builddir)/tools/mkclass/mkclass
	$(top_builddir)/tools/mkclass/mkclass $< > $@.tmp && mv $@.tmp $@

libcluster_la_SOURCES = \
	clusterlistener.cpp \
	clusterlistener.h \
	clusterlistener.ti \
	cluster-type.conf \
	endpoint.cpp \
	endpoint.h \
	endpoint.ti \
	jsonrpc.cpp \
	jsonrpc.h

libcluster_la_CPPFLAGS = \
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
