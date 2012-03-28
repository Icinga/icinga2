## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = base \
	jsonrpc \
	miniapp 

icinga2docdir = ${prefix}/doc/icinga2
icinga2doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(icinga2doc_DATA)


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(icinga2docdir)
