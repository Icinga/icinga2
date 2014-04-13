#!/bin/sh
ICINGA2PKIDIR=@CMAKE_INSTALL_FULL_DATADIR@/icinga2/pki

. $ICINGA2PKIDIR/pkifuncs

if [ -z "$1" ]; then
	echo "Syntax: $0 <csr-file>" >&2
	exit 1
fi

check_pki_dir

csrfile=$1

if [ ! -e "$ICINGA_CA/$csrfile" ]; then
	echo "The specified CSR file does not exist."
	exit 1
fi

pubkfile=${csrfile%.*}

if [ ! -f $ICINGA_CA/ca.crt -o ! -f $ICINGA_CA/ca.key ]; then
	echo "Please build a CA certificate first." >&2
	exit 1
fi

[ -f $ICINGA_CA/vars ] && . $ICINGA_CA/vars

openssl x509 -days "$REQ_DAYS" -CA $ICINGA_CA/ca.crt -CAkey $ICINGA_CA/ca.key -req -in $ICINGA_CA/$csrfile -outform PEM -out $ICINGA_CA/$pubkfile.crt -CAserial $ICINGA_CA/serial

# Make an agent bundle file
mkdir -p $ICINGA_CA/agent
cp $ICINGA_CA/$pubkfile.crt $ICINGA_CA/agent/agent.crt
cp $ICINGA_CA/ca.crt $ICINGA_CA/agent/ca.crt
tar cz -C $ICINGA_CA/agent/ ca.crt agent.crt | base64 > $ICINGA_CA/$pubkfile.bundle
rm -rf $ICINGA_CA/agent

echo "Done. $pubkfile.crt and $pubkfile.bundle files were written."
exit 0
