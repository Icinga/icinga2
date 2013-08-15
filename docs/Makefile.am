.PHONY: clean

EXTRA_DIST = \
	icinga2.adoc \
	icinga2-compat.adoc \
	icinga2-config.adoc \
	icinga2-config-syntax.adoc \
	icinga2-config-types.adoc \
	icinga2-install.adoc \
	icinga2-intro.adoc \
	icinga2-tutorial.adoc \
	icinga2-main.adoc \
	icinga2-migration.adoc \
	icinga2.8

icinga2docdir = ${docdir}
icinga2doc_DATA =

man8_MANS = \
	icinga2.8

.SUFFIXES = .html .adoc

if AD_COND_doc
icinga2doc_DATA += \
	icinga2.html \
	icinga2-compat.html \
	icinga2-config.html \
	icinga2-config-syntax.html \
	icinga2-config-types.html \
	icinga2-install.html \
	icinga2-intro.html \
	icinga2-tutorial.html \
	icinga2-main.html \
	icinga2-migration.html

.adoc.html:
	$(AD_ENV) $(AD_ASCIIDOC) -a toc -a numbered -o $@ $<
endif AD_COND_doc

icinga2.8: $(top_builddir)/icinga-app/icinga2
	-$(HELP2MAN) --name='The Icinga 2 networking monitoring daemon.' --section=8 --no-info --output=$@ $(top_builddir)/icinga-app/icinga2

# The icinga2.8 file is in EXTRA_DIST which means we need to build it when "make dist" is run.
# Unfortunately in order to build this man page we need to build Icinga 2. Meh.
$(top_builddir)/icinga-app/icinga2:
	$(MAKE) -C  $(top_builddir)

clean:
	rm -f $(icinga2doc_DATA)

distclean:
	rm -f $(icinga2doc_DATA)
