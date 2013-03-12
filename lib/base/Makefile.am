## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libbase.la

libbase_la_SOURCES =  \
	application.cpp \
	application.h \
	array.cpp \
	array.h \
	asynctask.h \
	attribute.cpp \
	attribute.h \
	connection.cpp \
	connection.h \
	convert.cpp \
	convert.h \
	dictionary.cpp \
	dictionary.h \
	dynamicobject.cpp \
	dynamicobject.h \
	dynamictype.cpp \
	dynamictype.h \
	eventqueue.cpp \
	eventqueue.h \
	exception.cpp \
	exception.h \
	fifo.cpp \
	fifo.h \
	i2-base.cpp \
	i2-base.h \
	logger.cpp \
	logger.h \
	netstring.cpp \
	netstring.h \
	object.cpp \
	object.h \
	objectlock.cpp \
	objectlock.h \
	process.cpp \
	process-unix.cpp \
	process-windows.cpp \
	process.h \
	qstring.cpp \
	qstring.h \
	ringbuffer.cpp \
	ringbuffer.h \
	script.cpp \
	script.h \
	scriptfunction.cpp \
	scriptfunction.h \
	scriptinterpreter.cpp \
	scriptinterpreter.h \
	scriptlanguage.cpp \
	scriptlanguage.h \
	scripttask.cpp \
	scripttask.h \
	socket.cpp \
	socket.h \
	stacktrace.cpp \
	stacktrace.h \
	stdiostream.cpp \
	stdiostream.h \
	stream.cpp \
	stream.h \
	stream_bio.cpp \
	stream_bio.h \
	streamlogger.cpp \
	streamlogger.h \
	sysloglogger.cpp \
	sysloglogger.h \
	tcpsocket.cpp \
	tcpsocket.h \
	timer.cpp \
	timer.h \
	tlsstream.cpp \
	tlsstream.h \
	unix.h \
	unixsocket.cpp \
	unixsocket.h \
	utility.cpp \
	utility.h \
	value.cpp \
	value.h \
	win32.h

libbase_la_CPPFLAGS = \
	-DI2_BASE_BUILD \
	$(LTDLINCL) \
	$(BOOST_CPPFLAGS) \
	$(OPENSSL_INCLUDES) \
	-I${top_srcdir}/third-party/execvpe \
	-I${top_srcdir}/third-party/mmatch \
	-I${top_srcdir}/third-party/cJSON

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
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/third-party/execvpe/libexecvpe.la \
	${top_builddir}/third-party/mmatch/libmmatch.la \
	${top_builddir}/third-party/cJSON/libcJSON.la
