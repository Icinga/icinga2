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

if ! openssl x509 -days "$REQ_DAYS" -CA $ICINGA_CA/ca.crt -CAkey $ICINGA_CA/ca.key -req -in $ICINGA_CA/$csrfile -outform PEM -out $ICINGA_CA/$pubkfile.crt -CAserial $ICINGA_CA/serial; then
	echo "Signing the CSR failed." >&2
	exit 1
fi

cn=`openssl x509 -in $pubkfile.crt -subject | grep -Eo '/CN=[^ ]+' | cut -f2- -d=`

case "$cn" in
  */*)
    echo "commonName contains invalid character (/)."
    exit 1
  ;;
esac


mv $pubkfile.crt $cn.crt
pubkfile=$cn

# Make an agent bundle file
tar cz -C $ICINGA_CA $pubkfile.crt ca.crt | base64 > $ICINGA_CA/$pubkfile.bundle

echo "Done. $pubkfile.crt and $pubkfile.bundle files were written."
exit 0
