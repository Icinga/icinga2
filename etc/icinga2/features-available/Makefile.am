## Process this file with automake to produce Makefile.in

icinga2confdir = $(sysconfdir)/icinga2/features-available

CONFIG_FILES = \
	compat.conf \
	ido-mysql.conf

install-data-local:
	@$(MKDIR_P) $(icinga2confdir); \
	for file in $(CONFIG_FILES); do \
	if [ ! -e $(icinga2confdir)/$$file ]; then \
	  $(INSTALL_DATA) $(srcdir)/$$file $(icinga2confdir)/$$file; \
	fi; \
	done

EXTRA_DIST = $(CONFIG_FILES)
