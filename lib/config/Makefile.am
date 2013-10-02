## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libconfig.la

BUILT_SOURCES = \
	config_parser.h

CLEANFILES = \
	base-type.cpp

AM_YFLAGS = -d

.conf.cpp: $(top_builddir)/tools/mkembedconfig/mkembedconfig
	$(top_builddir)/tools/mkembedconfig/mkembedconfig $< $@

.hh.h:
	cp $(srcdir)/$< $(srcdir)/$@

libconfig_la_SOURCES = \
	base-type.conf \
	configcompiler.cpp \
	configcompiler.h \
	configcompilercontext.cpp \
	configcompilercontext.h \
	config_lexer.ll \
	config_parser.yy \
	i2-config.h \
	configitem.cpp \
	configitem.h \
	configitembuilder.cpp \
	configitembuilder.h \
	configtype.cpp \
	configtype.h \
	debuginfo.h \
	expression.cpp \
	expression.h \
	expressionlist.cpp \
	expressionlist.h \
	typerule.cpp \
	typerule.h \
	typerulelist.cpp \
	typerulelist.h

libconfig_la_CPPFLAGS = \
	-DI2_CONFIG_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib

libconfig_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libconfig_la_LIBADD = \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/lib/base/libbase.la
