## Process this file with automake to produce Makefile.in

icinga2confdir = $(DESTDIR)${sysconfdir}/icinga2/features-available

CONFIG_FILES = \
	checker.conf \
	command.conf \
	compat-log.conf \
	ido-mysql.conf \
	livestatus.conf \
	notification.conf \
	perfdata.conf \
	syslog.conf \
	statusdat.conf

install-data-local:
	@$(MKDIR_P) $(icinga2confdir); \
	for file in $(CONFIG_FILES); do \
	if [ ! -e $(icinga2confdir)/$$file ]; then \
	  $(INSTALL_DATA) -o @icinga_user@ -g @icinga_group@ -m 0640 $(srcdir)/$$file $(icinga2confdir)/$$file; \
	fi; \
	done

EXTRA_DIST = $(CONFIG_FILES)
