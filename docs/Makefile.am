EXTRA_DIST = \
	icinga2-config.txt \
	icinga2-intro.txt

icinga2docdir = ${docdir}
icinga2doc_DATA = \
	icinga2-config.html \
	icinga2-intro.html

%.html: %.txt
	asciidoc -a toc -a numbered $^
