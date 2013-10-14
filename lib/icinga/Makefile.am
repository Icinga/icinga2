## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libicinga.la

CLEANFILES = \
	icinga-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

libicinga_la_SOURCES =  \
	api.cpp \
	api.h \
	checkcommand.cpp \
	checkcommand.h \
	cib.cpp \
	cib.h \
	command.cpp \
	command.h \
	compatutility.cpp \
	compatutility.h \
	domain.cpp \
	domain.h \
	eventcommand.cpp \
	eventcommand.h \
	externalcommandprocessor.cpp \
	externalcommandprocessor.h \
	host.cpp \
	hostgroup.cpp \
	hostgroup.h \
	host.h \
	i2-icinga.h \
	icinga-type.conf \
	icingaapplication.cpp \
	icingaapplication.h \
	legacytimeperiod.cpp \
	legacytimeperiod.h \
	macroprocessor.cpp \
	macroprocessor.h \
	macroresolver.cpp \
	macroresolver.h \
	notification.cpp \
	notification.h \
	notificationcommand.cpp \
	notificationcommand.h \
	nullchecktask.cpp \
	nullchecktask.h \
	nulleventtask.cpp \
	nulleventtask.h \
	pluginchecktask.cpp \
	pluginchecktask.h \
	plugineventtask.cpp \
	plugineventtask.h \
	pluginnotificationtask.cpp \
	pluginnotificationtask.h \
	service.cpp \
	service-check.cpp \
	service-comment.cpp \
	service-downtime.cpp \
	service-event.cpp \
	service-flapping.cpp \
	service-notification.cpp \
	service.h \
	servicegroup.cpp \
	servicegroup.h \
	timeperiod.cpp \
	timeperiod.h \
	user.cpp \
	user.h \
	usergroup.cpp \
	usergroup.h

libicinga_la_CPPFLAGS = \
	-DI2_ICINGA_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib

libicinga_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libicinga_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la
