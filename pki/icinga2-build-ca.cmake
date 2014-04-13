#!/bin/sh
ICINGA2PKIDIR=@CMAKE_INSTALL_FULL_DATADIR@/icinga2/pki

. $ICINGA2PKIDIR/pkifuncs

check_pki_dir

if [ `ls -1 -- $ICINGA_CA | wc -l` != 0 ]; then
	echo "The Icinga CA directory must be empty." >&2
	exit 1
fi

chmod 700 $ICINGA_CA >/dev/null 2>&1

echo '01' > $ICINGA_CA/serial
touch $ICINGA_CA/index.txt

cp $ICINGA2PKIDIR/vars $ICINGA_CA/
. $ICINGA_CA/vars

KEY_DIR=$ICINGA_CA openssl req -config $ICINGA2PKIDIR/openssl.cnf -new -newkey rsa:4096 -x509 -days 3650 -keyform PEM -keyout $ICINGA_CA/ca.key -outform PEM -out $ICINGA_CA/ca.crt && \
	chmod 600 $ICINGA_CA/ca.key && \
	echo -e "\n\tIf you want to change the default settings for server certificates check out \"$ICINGA_CA/vars\".\n"
