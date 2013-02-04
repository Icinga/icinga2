
all: create-docs

create-docs:
	for file in *.txt; \
	do asciidoc -a toc -a numbered $$file; \
	done
