## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	libnotification.la

EXTRA_DIST = \
	notification-type.conf

.conf.cpp: $(top_builddir)/tools/mkembedconfig
	$(top_builddir)/tools/mkembedconfig $< $@

libnotification_la_SOURCES = \
	notificationcomponent.cpp \
	notificationcomponent.h \
	notification-type.cpp \
	i2-notification.h

libnotification_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/remoting \
	-I${top_srcdir}/lib/icinga

libnotification_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libnotification_la_LIBADD = \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/remoting/libremoting.la \
	${top_builddir}/lib/icinga/libicinga.la
