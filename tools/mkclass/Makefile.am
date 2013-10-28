## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = \
	mkclass

EXTRA_DIST = \
	class_lexer.cc \
	class_parser.cc \
	class_parser.h

AM_YFLAGS = -d

.hh.h:
	if [ -e $(srcdir)/$< ]; then cp $(srcdir)/$< $(srcdir)/$@; fi

class_lexer.cc: class_parser.h

mkclass_SOURCES = \
	classcompiler.cpp \
	classcompiler.h \
	class_lexer.ll \
	class_parser.yy \
	mkclass.cpp
