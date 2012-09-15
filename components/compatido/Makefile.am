## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	compatido.la

compatido_la_SOURCES = \
	compatidocomponent.cpp \
	compatidocomponent.h \
	i2-compatido.h

compatido_la_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        -I${top_srcdir}/base \
        -I${top_srcdir}/dyn \
        -I${top_srcdir}/jsonrpc \
        -I${top_srcdir}/icinga \
        -I${top_srcdir}/cib

compatido_la_LDFLAGS = \
        $(BOOST_LDFLAGS) \
        -module \
        -no-undefined \
        @RELEASE_INFO@ \
        @VERSION_INFO@

compatido_la_LIBADD = \
        $(BOOST_SIGNALS_LIB) \
        $(BOOST_THREAD_LIB) \
        ${top_builddir}/base/libbase.la \
        ${top_builddir}/dyn/libdyn.la \
        ${top_builddir}/jsonrpc/libjsonrpc.la \
        ${top_builddir}/icinga/libicinga.la \
        ${top_builddir}/cib/libcib.la

