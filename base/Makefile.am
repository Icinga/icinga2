## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libbase.la

libbase_la_SOURCES =  \
	application.cpp \
	application.h \
	asynctask.cpp \
	asynctask.h \
	component.cpp \
	component.h \
	configobject.cpp \
	configobject.h \
	dictionary.cpp \
	dictionary.h \
	event.cpp \
	event.h \
	exception.cpp \
	exception.h \
	fifo.cpp \
	fifo.h \
	i2-base.h \
	logger.cpp \
	logger.h \
	object.cpp \
	object.h \
	objectset.cpp \
	objectset.h \
	objectmap.cpp \
	objectmap.h \
	process.cpp \
	process.h \
	ringbuffer.cpp \
	ringbuffer.h \
	scriptfunction.cpp \
	scriptfunction.h \
	scripttask.cpp \
	scripttask.h \
	socket.cpp \
	socket.h \
	streamlogger.cpp \
	streamlogger.h \
	sysloglogger.cpp \
	sysloglogger.h \
	tcpclient.cpp \
	tcpclient.h \
	tcpserver.cpp \
	tcpserver.h \
	tcpsocket.cpp \
	tcpsocket.h \
	threadpool.cpp \
	threadpool.h \
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
	-I${top_srcdir}/third-party/mmatch \
	-I${top_srcdir}/third-party/popen-noshell

libbase_la_LDFLAGS = \
	$(BOOST_LDFLAGS) \
	$(OPENSSL_LDFLAGS) \
	-no-undefined \
	@RELEASE_INFO@ \
	@VERSION_INFO@

libbase_la_LIBADD = \
	$(LIBLTDL) \
	$(OPENSSL_LIBS) \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	${top_builddir}/third-party/mmatch/libmmatch.la \
	${top_builddir}/third-party/popen-noshell/libpopen_noshell.la
