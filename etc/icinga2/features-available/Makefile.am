## Process this file with automake to produce Makefile.in

icinga2confdir = $(DESTDIR)${sysconfdir}/icinga2/features-available

CONFIG_FILES = \
	checker.conf \
	command.conf \
	compatlog.conf \
	debuglog.conf \
	graphite.conf \
	ido-mysql.conf \
	livestatus.conf \
	notification.conf \
	perfdata.conf \
	syslog.conf \
	statusdata.conf

install-data-local:
	@$(MKDIR_P) $(icinga2confdir); \
	for file in $(CONFIG_FILES); do \
	if [ ! -e $(icinga2confdir)/$$file ]; then \
	  $(INSTALL_DATA) $(INSTALL_OPTS) -m 0640 $(srcdir)/$$file $(icinga2confdir)/$$file; \
	fi; \
	done

uninstall-hook:
	for file in $(CONFIG_FILES); do \
	  rm -f $(icinga2confdir)/$$file; \
	done; \
	rmdir $(icinga2confdir)

EXTRA_DIST = $(CONFIG_FILES)
