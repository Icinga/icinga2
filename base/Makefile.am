## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libbase.la

libbase_la_SOURCES =  \
	application.cpp \
	application.h \
	component.cpp \
	component.h \
	condvar.cpp \
	condvar.h \
	configcollection.cpp \
	configcollection.h \
	confighive.cpp \
	confighive.h \
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
	memory.cpp \
	memory.h \
	mutex.cpp \
	mutex.h \
	object.cpp \
	object.h \
	socket.cpp \
	socket.h \
	tcpclient.cpp \
	tcpclient.h \
	tcpserver.cpp \
	tcpserver.h \
	tcpsocket.cpp \
	tcpsocket.h \
	thread.cpp \
	thread.h \
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

libbase_la_CXXFLAGS = \
	-DI2_BASE_BUILD \
	$(LTDLINCL) \
	-I${top_srcdir}/mmatch

libbase_la_LDFLAGS = \
	-no-undefined \
	-version-info 0:0:0

libbase_la_LIBADD = \
	$(LIBLTDL) \
	${top_builddir}/mmatch/libmmatch.la
