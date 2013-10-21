## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = \
	icinga2

icinga2_SOURCES = \
	icinga.cpp

icinga2_CPPFLAGS = \
	-DI2_ICINGALAUNCHER_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib \
	-I${top_srcdir}/components

icinga2_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-export-dynamic

icinga2_LDADD = \
	$(LIBLTDL) \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	${top_builddir}/lib/base/libbase.la \
	${top_builddir}/lib/config/libconfig.la \
	-dlopen ${top_builddir}/lib/icinga/libicinga.la \
	-dlopen ${top_builddir}/components/checker/libchecker.la \
	-dlopen ${top_builddir}/components/cluster/libcluster.la \
	-dlopen ${top_builddir}/components/compat/libcompat.la \
	-dlopen ${top_builddir}/components/demo/libdemo.la \
	-dlopen ${top_builddir}/components/livestatus/liblivestatus.la \
	-dlopen ${top_builddir}/components/notification/libnotification.la \
	-dlopen ${top_builddir}/components/perfdata/libperfdata.la

if PYTHON_USE
icinga2_LDADD += \
	-dlopen ${top_builddir}/lib/python/libpython.la
endif

if MYSQL_USE
icinga2_LDADD += \
	-dlopen ${top_builddir}/components/db_ido_mysql/libdb_ido_mysql.la
endif
