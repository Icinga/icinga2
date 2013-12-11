#!/bin/bash
ICINGA2PKIDIR=@CMAKE_INSTALL_FULL_DATADIR@/icinga2/pki

source $ICINGA2PKIDIR/pkifuncs

if [ -z "$1" ]; then
	echo "Syntax: $0 <name>" >&2
	exit 1
fi

name=$1

check_pki_dir

if [ ! -f $ICINGA_CA/ca.crt -o ! -f $ICINGA_CA/ca.key ]; then
	echo "Please build a CA certificate first." >&2
	exit 1
fi

[ -f $ICINGA_CA/vars ] && source $ICINGA_CA/vars

[ -z "$REQ_COUNTRY_NAME" ] && export REQ_COUNTRY_NAME="AU"
[ -z "$REQ_STATE" ] && export REQ_STATE="Some-State"
[ -z "$REQ_ORGANISATION" ] && export REQ_ORGANISATION="Internet Widgits Pty Ltd"
[ -z "$REQ_ORG_UNIT" ] && export REQ_ORG_UNIT="Monitoring"
[ -z "$REQ_COMMON_NAME" ] && export REQ_COMMON_NAME="Icinga CA"
[ -z "$REQ_DAYS" ] && export REQ_DAYS="3650"

REQ_COMMON_NAME="$name" KEY_DIR="$ICINGA_CA" openssl req -config $ICINGA2PKIDIR/openssl.cnf -new -newkey rsa:4096 -keyform PEM -keyout $ICINGA_CA/$name.key -outform PEM -out $ICINGA_CA/$name.csr -nodes && \
	openssl x509 -days "$REQ_DAYS" -CA $ICINGA_CA/ca.crt -CAkey $ICINGA_CA/ca.key -req -in $ICINGA_CA/$name.csr -outform PEM -out $ICINGA_CA/$name.tmp -CAserial $ICINGA_CA/serial && \
	chmod 600 $ICINGA_CA/$name.key && \
	openssl x509 -in $ICINGA_CA/$name.tmp -text >  $ICINGA_CA/$name.crt && \
	rm -f $ICINGA_CA/$name.csr $ICINGA_CA/$name.tmp
