SUBDIRS = \
	strapdownjs

MARKDOWN_FILES = \
	1-about.md \
	2.0-getting-started.md \
	2.1-setting-up-icinga-2.md \
	2.2-setting-up-icinga-classic-ui.md \
	2.3-setting-up-check-plugins.md \
	2.4-setting-up-ido.md \
	2.5-setting-up-livestatus.md \
	2.6-running-icinga.md \
	3.01-hosts-and-services.md \
	3.02-commands.md \
	3.03-macros.md \
	3.04-notifications.md \
	3.05-using-templates.md \
	3.06-groups.md \
	3.07-time-periods.md \
	3.08-external-commands.md \
	3.09-event-handlers.md \
	3.10-logging.md \
	3.11-performance-data.md \
	3.12-status-data.md \
	3.13-compat-logging.md \
	3.14-check-result-files.md \
	3-monitoring-basics.md \
	4.1-configuration-syntax.md \
	4.2-global-variables.md \
	4.3-object-types.md \
	4-configuring-icinga.md \
	5-icinga-template-library.md \
	6-advanced-topics.md \
	7-migrating-from-icinga-1x.md \
	8-differences-between-icinga-1x-and-2.md \
	9-vagrant-demo-vm.md

DOCVIEWER_FILES = \
	jquery.min.js \
	jquery.toc.min.js \
	toc.css \
	toc-margin.css \
	toc-print.css

EXTRA_DIST = \
	$(MARKDOWN_FILES) \
	$(DOCVIEWER_FILES) \
	icinga2.8 \
	mkdoc.sh \
	BLACKLIST

CLEANFILES = \
	index.html

icinga2docdir = ${docdir}/htdocs
icinga2doc_DATA = \
	index.html \
	$(DOCVIEWER_FILES)

index.html: $(MARKDOWN_FILES)
	$(srcdir)/mkdoc.sh > $@.tmp && mv $@.tmp $@

man8_MANS = \
	icinga2.8

