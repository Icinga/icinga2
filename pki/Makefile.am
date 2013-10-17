bin_SCRIPTS = \
	icinga2-build-ca \
	icinga2-build-key

icinga2pkidir = ${pkgdatadir}/pki
icinga2pki_DATA = \
	pkifuncs \
	openssl.cnf

CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = \
	icinga2-build-ca.in \
	icinga2-build-key.in \
	$(icinga2pki_DATA)
