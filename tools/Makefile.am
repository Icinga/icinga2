SUBDIRS = \
		  mkembedconfig

bin_SCRIPTS = i2enfeature
CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = i2enfeature.in

install-exec-hook:
	$(LN_S) -f i2enfeature $(bindir)/i2disfeature
