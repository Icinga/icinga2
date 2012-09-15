## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	compatido.la

compatido_la_SOURCES = \
	i2-compatido.h \
	idosocket.cpp \
	idosocket.h \
	compatidocomponent.cpp \
	compatidocomponent.h

compatido_la_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        -I${top_srcdir}/lib/base \
        -I${top_srcdir}/lib/config \
        -I${top_srcdir}/lib/remoting \
        -I${top_srcdir}/lib/icinga

compatido_la_LDFLAGS = \
        $(BOOST_LDFLAGS) \
        -module \
        -no-undefined \
        @RELEASE_INFO@ \
        @VERSION_INFO@

compatido_la_LIBADD = \
        $(BOOST_SIGNALS_LIB) \
        $(BOOST_THREAD_LIB) \
        ${top_builddir}/lib/base/libbase.la \
        ${top_builddir}/lib/config/libconfig.la \
        ${top_builddir}/lib/remoting/libremoting.la \
        ${top_builddir}/lib/icinga/libicinga.la

