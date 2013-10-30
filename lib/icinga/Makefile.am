## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libicinga.la

BUILT_SOURCES = \
	checkcommand.th \
	command.th \
	domain.th \
	eventcommand.th \
	host.th \
	hostgroup.th \
	icingaapplication.th \
	notification.th \
	notificationcommand.th \
	service.th \
	servicegroup.th \
	timeperiod.th \
	user.th \
	usergroup.th

nodist_libicinga_la_SOURCES = $(BUILT_SOURCES)

CLEANFILES = \
	icinga-type.cpp

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

.ti.th: $(top_builddir)/tools/mkclass/mkclass
	$(top_builddir)/tools/mkclass/mkclass $< > $@.tmp && mv $@.tmp $@

libicinga_la_SOURCES =  \
	api.cpp \
	api.h \
	checkcommand.cpp \
	checkcommand.h \
	checkcommand.ti \
	cib.cpp \
	cib.h \
	command.cpp \
	command.h \
	command.ti \
	compatutility.cpp \
	compatutility.h \
	domain.cpp \
	domain.h \
	domain.ti \
	eventcommand.cpp \
	eventcommand.h \
	eventcommand.ti \
	externalcommandprocessor.cpp \
	externalcommandprocessor.h \
	host.cpp \
	host.h \
	host.ti \
	hostgroup.cpp \
	hostgroup.h \
	hostgroup.ti \
	i2-icinga.h \
	icinga-type.conf \
	icingaapplication.cpp \
	icingaapplication.h \
	icingaapplication.ti \
	legacytimeperiod.cpp \
	legacytimeperiod.h \
	macroprocessor.cpp \
	macroprocessor.h \
	macroresolver.cpp \
	macroresolver.h \
	notification.cpp \
	notification.h \
	notification.ti \
	notificationcommand.cpp \
	notificationcommand.h \
	notificationcommand.ti \
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
	randomchecktask.cpp \
	randomchecktask.h \
	service.cpp \
	service.h \
	service.ti \
	service-check.cpp \
	service-comment.cpp \
	service-downtime.cpp \
	service-event.cpp \
	service-flapping.cpp \
	service-notification.cpp \
	servicegroup.cpp \
	servicegroup.h \
	servicegroup.ti \
	timeperiod.cpp \
	timeperiod.h \
	timeperiod.ti \
	user.cpp \
	user.h \
	user.ti \
	usergroup.cpp \
	usergroup.h \
	usergroup.ti

libicinga_la_CPPFLAGS = \
	-DI2_ICINGA_BUILD \
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
