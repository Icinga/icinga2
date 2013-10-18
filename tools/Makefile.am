SUBDIRS = \
	  migration \
	  mkembedconfig

sbin_SCRIPTS = icinga2-enable-feature
CLEANFILES = $(sbin_SCRIPTS)

EXTRA_DIST = icinga2-enable-feature.in

install-exec-hook:
	$(LN_S) -f icinga2-enable-feature $(DESTDIR)$(sbindir)/icinga2-disable-feature
