## Process this file with automake to produce Makefile.in

icinga2confdir = $(sysconfdir)/icinga2/mods-available
icinga2conf_DATA = \
	compat.load

CONFIG_FILES = \
	compat.conf

install-data-local:
	@for file in $(CONFIG_FILES); do \
	if [ ! -e $(icinga2confdir)/$$file ]; then \
	  $(INSTALL_DATA) $$file $(icinga2confdir)/$$file; \
	fi; \
	done

EXTRA_DIST = $(CONFIG_FILES) $(icinga2conf_DATA)
