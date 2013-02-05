.PHONY: clean

EXTRA_DIST = \
	icinga2-config.txt \
	icinga2-intro.txt

icinga2docdir = ${docdir}
icinga2doc_DATA = \
	icinga2-config.html \
	icinga2-intro.html

.SUFFIXES = .html .txt

.txt.html:
	asciidoc -a toc -a numbered $<

clean:
	rm -f $(icinga2doc_DATA)
