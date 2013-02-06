.PHONY: clean

EXTRA_DIST = \
	icinga2-config.txt \
	icinga2-intro.txt \
	icinga2.8

icinga2docdir = ${docdir}
icinga2doc_DATA = \
	icinga2-config.html \
	icinga2-intro.html

man1_MANS = \
	icinga2.8

.SUFFIXES = .html .txt

.txt.html:
	asciidoc -a toc -a numbered $<

icinga2.8: $(top_srcdir)/icinga-app/icinga2
	-$(HELP2MAN) --name='The Icinga 2 networking monitoring daemon.' --section=8 --no-info --output=$@ $(top_srcdir)/icinga-app/icinga2

distclean:
	rm -f $(icinga2doc_DATA)
