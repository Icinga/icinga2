#!/bin/bash
ICINGA2PKIDIR=@CMAKE_INSTALL_FULL_DATADIR@/icinga2/pki

source $ICINGA2PKIDIR/pkifuncs

if [ -z "$1" ]; then
	echo "Syntax: $0 <csr-file>" >&2
	exit 1
fi

csrfile=$1

if [ ! -e "$csrfile" ]; then
	echo "The specified CSR file does not exist."
	exit 1
fi

pubkfile=$(basename -- ${csrfile%.*})

check_pki_dir

if [ ! -f $ICINGA_CA/ca.crt -o ! -f $ICINGA_CA/ca.key ]; then
	echo "Please build a CA certificate first." >&2
	exit 1
fi

[ -f $ICINGA_CA/vars ] && source $ICINGA_CA/vars

openssl x509 -days "$REQ_DAYS" -CA $ICINGA_CA/ca.crt -CAkey $ICINGA_CA/ca.key -req -in $ICINGA_CA/$csrfile -outform PEM -out $ICINGA_CA/$csrfile.tmp -CAserial $ICINGA_CA/serial && \
	openssl x509 -in $ICINGA_CA/$csrfile.tmp -text > $ICINGA_CA/$pubkfile.crt && \
	rm -f $ICINGA_CA/$csrfile.tmp

# Make an agent bundle file
mkdir -p $ICINGA_CA/agent
cp $ICINGA_CA/$pubkfile.crt $ICINGA_CA/agent/agent.crt
cp $ICINGA_CA/ca.crt $ICINGA_CA/agent/ca.crt
tar cf $ICINGA_CA/$pubkfile.bundle -C $ICINGA_CA/agent/ ca.crt agent.crt
rm -rf $ICINGA_CA/agent
