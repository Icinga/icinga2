SUBDIRS = \
		  mkembedconfig

bin_SCRIPTS = i2modconf
CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = i2modconf.in

install-exec-hook:
	$(LN_S) -f i2modconf $(bindir)/i2enmod
	$(LN_S) -f i2modconf $(bindir)/i2dismod
