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
	$(AD_ENV) $(AD_ASCIIDOC) -d book -a toc -a numbered -o $@ $<
endif AD_COND_doc

clean:
	rm -f $(icinga2doc_DATA)

distclean:
	rm -f $(icinga2doc_DATA)
