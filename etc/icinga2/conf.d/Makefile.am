## Process this file with automake to produce Makefile.in

icinga2confdir = $(DESTDIR)${sysconfdir}/icinga2/conf.d

CONFIG_FILES = \
	generic-host.conf \
	generic-service.conf \
	groups.conf \
	localhost.conf \
	macros.conf \
	notifications.conf \
	timeperiods.conf \
	users.conf

install-data-local:
	@if [ ! -e $(icinga2confdir) ]; then \
	  $(MKDIR_P) $(icinga2confdir); \
	  for file in $(CONFIG_FILES); do \
	    $(INSTALL_DATA) $(INSTALL_OPTS) -m 0640 $(srcdir)/$$file $(icinga2confdir)/$$file; \
	  done; \
	fi

uninstall-hook:
	@for file in $(CONFIG_FILES); do \
	  rm -f $(icinga2confdir)/$$file; \
	done; \
	rmdir $(icinga2confdir)

EXTRA_DIST = $(CONFIG_FILES)
