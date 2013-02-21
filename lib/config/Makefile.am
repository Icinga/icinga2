## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES = \
	libconfig.la

BUILT_SOURCES = config_parser.h

AM_YFLAGS = -d

libconfig_la_SOURCES = \
	configcompiler.cpp \
	configcompiler.h \
	configcompilercontext.cpp \
	configcompilercontext.h \
	config_lexer.ll \
	config_parser.yy \
	i2-config.cpp \
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
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/lib/base

libconfig_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libconfig_la_LIBADD = \
	${top_builddir}/lib/base/libbase.la

if USE_PCH
BUILT_SOURCES += i2-config.h.gch

i2-config.h.gch: i2-config.h
	$(AM_V_CXX)$(LTCXXCOMPILE) $(libconfig_la_CPPFLAGS) -o $@ $^

libconfig_la_DEPENDENCIES = \
	i2-config.h.gch

clean-local:
	rm -f i2-config.h.gch
endif

