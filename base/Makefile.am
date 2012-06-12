## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libbase.la

libbase_la_SOURCES =  \
	application.cpp \
	application.h \
	component.cpp \
	component.h \
	configobject.cpp \
	configobject.h \
	cxx11-compat.h \
	delegate.h \
	dictionary.cpp \
	dictionary.h \
	observable.h \
	exception.cpp \
	exception.h \
	fifo.cpp \
	fifo.h \
	i2-base.h \
	object.cpp \
	object.h \
	objectset.cpp \
	objectset.h \
	objectmap.cpp \
	objectmap.h \
	socket.cpp \
	socket.h \
	tcpclient.cpp \
	tcpclient.h \
	tcpserver.cpp \
	tcpserver.h \
	tcpsocket.cpp \
	tcpsocket.h \
	timer.cpp \
	timer.h \
	tlsclient.cpp \
	tlsclient.h \
	unix.cpp \
	unix.h \
	utility.cpp \
	utility.h \
	variant.cpp \
	variant.h \
	win32.cpp \
	win32.h

libbase_la_CPPFLAGS = \
	-DI2_BASE_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	$(OPENSSL_INCLUDES) \
	-I${top_srcdir}/third-party/mmatch

libbase_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(OPENSSL_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libbase_la_LIBADD = \
	$(LIBLTDL) \
	$(OPENSSL_LIBS) \
	${top_builddir}/third-party/mmatch/libmmatch.la
