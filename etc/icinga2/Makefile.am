## Process this file with automake to produce Makefile.in

SUBDIRS = \
	conf.d \
	features-available

icinga2confdir = $(DESTDIR)${sysconfdir}/icinga2

CONFIG_FILES = \
	icinga2.conf

install-data-local:
	@for file in $(CONFIG_FILES); do \
	if [ ! -e $(icinga2confdir)/$$file ]; then \
	  $(INSTALL_DATA) $(INSTALL_OPTS) -m 0640 $(srcdir)/$$file $(icinga2confdir)/$$file; \
	fi; \
	done

EXTRA_DIST = $(CONFIG_FILES)

install-exec-hook:
	@cd $(icinga2confdir) && \
	if [ ! -e features-enabled ]; then \
	  $(MKDIR_P) features-enabled && \
	  $(LN_S) ../features-available/checker.conf features-enabled/; \
	  $(LN_S) ../features-available/notification.conf features-enabled/; \
	  $(LN_S) ../features-available/syslog.conf features-enabled/; \
	fi

uninstall-hook:
	rm -f $(icinga2confdir)/features-enabled/checker.conf
	rm -f $(icinga2confdir)/features-enabled/notification.conf
	rm -f $(icinga2confdir)/features-enabled/syslog.conf
	rmdir $(icinga2confdir)/features-enabled
	@for file in $(CONFIG_FILES); do \
	  rm -f $(icinga2confdir)/$$file; \
	done; \
	rmdir $(icinga2confdir)
