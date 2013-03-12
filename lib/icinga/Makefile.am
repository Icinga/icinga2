## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libicinga.la

EXTRA_DIST = \
	icinga-type.conf

.conf.cpp:
	$(top_builddir)/tools/mkembedconfig $< $@

libicinga_la_SOURCES =  \
	api.cpp \
	api.h \
	checkresultmessage.cpp \
	checkresultmessage.h \
	cib.cpp \
	cib.h \
	externalcommandprocessor.cpp \
	externalcommandprocessor.h \
	host.cpp \
	hostgroup.cpp \
	hostgroup.h \
	host.h \
	i2-icinga.cpp \
	i2-icinga.h \
	icinga-type.cpp \
	icingaapplication.cpp \
	icingaapplication.h \
	macroprocessor.cpp \
	macroprocessor.h \
	notification.cpp \
	notification.h \
	notificationrequestmessage.cpp \
	notificationrequestmessage.h \
	nullchecktask.cpp \
	nullchecktask.h \
	perfdatawriter.cpp \
	perfdatawriter.h \
	pluginchecktask.cpp \
	pluginchecktask.h \
	pluginnotificationtask.cpp \
	pluginnotificationtask.h \
	service.cpp \
	service-check.cpp \
	service-comment.cpp \
	service-downtime.cpp \
	service-notification.cpp \
	service.h \
	servicegroup.cpp \
	servicegroup.h \
	user.cpp \
	user.h \
	usergroup.cpp \
	usergroup.h

libicinga_la_CPPFLAGS = \
	-DI2_ICINGA_BUILD \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base \
	-I${top_srcdir}/lib/config \
	-I${top_srcdir}/lib/remoting \
	-I${top_srcdir}

libicinga_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libicinga_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	${top_builddir}/lib/remoting/libremoting.la
