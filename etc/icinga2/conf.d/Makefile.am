## Process this file with automake to produce Makefile.in

icinga2confdir = $(sysconfdir)/icinga2/conf.d

CONFIG_FILES = \
	localhost.conf \
	macros.conf

install-data-local:
	@$(MKDIR_P) $(icinga2confdir); \
	for file in $(CONFIG_FILES); do \
	if [ ! -e $(icinga2confdir)/$$file ]; then \
	  $(INSTALL_DATA) -o @icinga_user@ -g @icinga_group@ -m 0640 $(srcdir)/$$file $(icinga2confdir)/$$file; \
	fi; \
	done

EXTRA_DIST = $(CONFIG_FILES)
