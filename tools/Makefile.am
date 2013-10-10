SUBDIRS = \
	  migration \
	  mkembedconfig

sbin_SCRIPTS = i2enfeature
CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = i2enfeature.in

install-exec-hook:
	$(LN_S) -f i2enfeature $(DESTDIR)$(sbindir)/i2disfeature
