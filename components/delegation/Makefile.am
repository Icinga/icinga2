## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = \
	delegation.la

delegation_la_SOURCES = \
	delegationcomponent.cpp \
	delegationcomponent.h \
	i2-delegation.h

delegation_la_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	-I${top_srcdir}/base \
	-I${top_srcdir}/jsonrpc \
	-I${top_srcdir}/icinga \
	-I${top_srcdir}/cib

delegation_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	-module \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

delegation_la_LIBADD = \
	${top_builddir}/base/libbase.la \
	${top_builddir}/jsonrpc/libjsonrpc.la \
	${top_builddir}/icinga/libicinga.la \
	${top_builddir}/cib/libcib.la
