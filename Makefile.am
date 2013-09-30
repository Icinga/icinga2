## Process this file with automake to produce Makefile.in
## Created by Anjuta

ACLOCAL_AMFLAGS = -I m4

include aminclude.am

SUBDIRS = \
	third-party \
	tools \
	lib \
	components \
	icinga-app \
	test \
	itl \
	etc \
	docs

#doc
icinga2docdir = ${docdir}
icinga2doc_DATA = \
	README \
	COPYING \
	COPYING.Exceptions \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS

install-data-local:
	$(MKDIR_P) $(DESTDIR)${localstatedir}/log/${PACKAGE}/compat/archives
	$(MKDIR_P) $(DESTDIR)${localstatedir}/cache/${PACKAGE}
	$(MKDIR_P) $(DESTDIR)${localstatedir}/spool/${PACKAGE}/perfdata
	$(MKDIR_P) $(DESTDIR)${localstatedir}/lib/${PACKAGE}/cluster/config
	$(MKDIR_P) $(DESTDIR)${localstatedir}/lib/${PACKAGE}/cluster/log
	$(MKDIR_P) $(DESTDIR)${localstatedir}/lib/${PACKAGE}
	$(MKDIR_P) $(DESTDIR)${localstatedir}/run/${PACKAGE}
	$(INSTALL) -m 775 -d $(COMMAND_OPTS) $(DESTDIR)${localstatedir}/run/${PACKAGE}/cmd
	chmod g+s $(DESTDIR)${localstatedir}/run/${PACKAGE}/cmd

EXTRA_DIST = $(icinga2doc_DATA) git_version.sh icinga2.spec


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

check-coverage: all
	lcov -d . -z
	make check
	lcov -d . -c -o icinga2.info
	lcov -e icinga2.info -o icinga2-filtered.info "`readlink -f $(top_srcdir)`/*"
	mkdir -p docs/lcov
	genhtml -o docs/lcov icinga2-filtered.info

