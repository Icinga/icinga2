## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = ltdl \
	base \
	jsonrpc \
	configfilecomponent \
	configrpccomponent \
	icinga

icinga2docdir = ${prefix}/doc/icinga2
icinga2doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(icinga2doc_DATA)


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(icinga2docdir)

BUILT_SOURCES = icinga-version.stamp
CLEANFILES = icinga-version.h
GIT_VERSION_CMD = $(SHELL) $(top_srcdir)/git_version.sh
icinga-version.stamp:
	@if test -f "$(srcdir)/icinga-version.h"; then \
		if test -f "icinga-version.h"; then :; \
		else \
			cp "$(srcdir)/icinga-version.h" "icinga-version.h"; \
		fi; \
	fi
	$(GIT_VERSION_CMD) -k -s $(top_srcdir) -o icinga-version.h
	@if test -s "$(srcdir)/icinga-version.h"; then \
		if cmp "$(srcdir)/icinga-version.h" "icinga-version.h"; then :; \
		else \
			echo "Error: $(srcdir)/icinga-version.h and icinga-version.h differ."; \
			echo "       You probably want to remove the former."; \
			exit 1; \
		fi; \
	fi

dist-hook: icinga-version.stamp
	if test -f "icinga-version.h"; then \
		$(SED) -e 's|^#undef GIT_IS_DIST.*|#define GIT_IS_DIST 1|' \
			"icinga-version.h" > "$(distdir)/icinga-version.h"; \
	fi