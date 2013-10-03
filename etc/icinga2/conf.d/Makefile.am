## Process this file with automake to produce Makefile.in

icinga2confdir = $(DESTDIR)${sysconfdir}/icinga2/conf.d

CONFIG_FILES = \
	localhost.conf \
	macros.conf

install-data-local:
	@if [ ! -e $(icinga2confdir) ]; then \
	  $(MKDIR_P) $(icinga2confdir); \
	  for file in $(CONFIG_FILES); do \
	    $(INSTALL_DATA) -o @icinga_user@ -g @icinga_group@ -m 0640 $(srcdir)/$$file $(icinga2confdir)/$$file; \
	  done; \
	fi

uninstall-hook:
	@for file in $(CONFIG_FILES); do \
	  rm -f $(icinga2confdir)/$$file; \
	done; \
	rmdir $(icinga2confdir)

EXTRA_DIST = $(CONFIG_FILES)
