## Process this file with automake to produce Makefile.in


pkglib_LTLIBRARIES =  \
	libbase.la

BUILT_SOURCES = \
	application.th \
	consolelogger.th \
	dynamicobject.th \
	filelogger.th \
	logger.th \
	script.th \
	streamlogger.th \
	sysloglogger.th

nodist_libbase_la_SOURCES = $(BUILT_SOURCES)

.ti.th: $(top_builddir)/tools/mkclass/mkclass
	$(top_builddir)/tools/mkclass/mkclass $< > $@.tmp && mv $@.tmp $@

libbase_la_SOURCES =  \
	application.cpp \
	application.h \
	application.ti \
	array.cpp \
	array.h \
	bufferedstream.cpp \
	bufferedstream.h \
	consolelogger.cpp \
	consolelogger.h \
	consolelogger.ti \
	convert.cpp \
	convert.h \
	debug.h \
	dictionary.cpp \
	dictionary.h \
	dynamicobject.cpp \
	dynamicobject.h \
	dynamicobject.ti \
	dynamictype.cpp \
	dynamictype.h \
	exception.cpp \
	exception.h \
	fifo.cpp \
	fifo.h \
	filelogger.cpp \
	filelogger.h \
	filelogger.ti \
	i2-base.h \
	initialize.h \
	logger.cpp \
	logger.h \
	logger.ti \
	logger_fwd.h \
	netstring.cpp \
	netstring.h \
	networkstream.cpp \
	networkstream.h \
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
	reflectionobject.cpp \
	reflectionobject.h \
	registry.h \
	ringbuffer.cpp \
	ringbuffer.h \
	script.cpp \
	script.h \
	script.ti \
	scriptfunction.cpp \
	scriptfunction.h \
	scriptfunctionwrapper.cpp \
	scriptfunctionwrapper.h \
	scriptinterpreter.cpp \
	scriptinterpreter.h \
	scriptlanguage.cpp \
	scriptlanguage.h \
	scriptvariable.cpp \
	scriptvariable.h \
	singleton.h \
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
	streamlogger.ti \
	sysloglogger.cpp \
	sysloglogger.h \
	sysloglogger.ti \
	tcpsocket.cpp \
	tcpsocket.h \
	threadpool.cpp \
	threadpool.h \
	timer.cpp \
	timer.h \
	tlsstream.cpp \
	tlsstream.h \
	tlsutility.cpp \
	tlsutility.h \
	unix.h \
	unixsocket.cpp \
	unixsocket.h \
	utility.cpp \
	utility.h \
	value.cpp \
	value.h \
	win32.h \
	workqueue.cpp \
	workqueue.h \
	zlibstream.cpp \
	zlibstream.h

libbase_la_CPPFLAGS = \
	-DI2_BASE_BUILD \
	$(BOOST_CPPFLAGS) \
	$(OPENSSL_INCLUDES) \
	-I${top_srcdir}/lib \
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
	$(LIBDL) \
	$(OPENSSL_LIBS) \
	$(BOOST_SIGNALS_LIB) \
	$(BOOST_THREAD_LIB) \
	$(BOOST_SYSTEM_LIB) \
	${top_builddir}/third-party/execvpe/libexecvpe.la \
	${top_builddir}/third-party/mmatch/libmmatch.la \
	${top_builddir}/third-party/cJSON/libcJSON.la
