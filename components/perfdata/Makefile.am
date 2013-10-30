## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libperfdata.la

BUILT_SOURCES = \
	graphitewriter.th \
	perfdatawriter.th

nodist_libperfdata_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES = \
	perfdata-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

.ti.th: $(top_builddir)/tools/mkclass/mkclass
	$(top_builddir)/tools/mkclass/mkclass $< > $@.tmp && mv $@.tmp $@

libperfdata_la_SOURCES =  \
	graphitewriter.cpp \
	graphitewriter.h \
	graphitewriter.ti \
	perfdatawriter.cpp \
	perfdatawriter.h \
	perfdatawriter.ti \
	perfdata-type.conf

libperfdata_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

libperfdata_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libperfdata_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/icinga/libicinga.la
