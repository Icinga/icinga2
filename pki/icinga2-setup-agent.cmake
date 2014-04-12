#!/bin/bash
ICINGA2PKIDIR=@CMAKE_INSTALL_FULL_DATADIR@/icinga2/pki
ICINGA2CONFIG=@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2

if [ -n "$1" ]; then
	if [ ! -e $ICINGA2CONFIG/pki/agent/agent.key ]; then
		echo "You haven't generated a private key for this Icinga 2 instance"
		echo "yet. Please run this script without any parameters to generate a key."
		exit 1
	fi

	if [ ! -e "$1" ]; then
		echo "The specified key bundle does not exist."
		exit 1
	fi

	echo "Installing the certificate bundle..."
	tar -C $ICINGA2CONFIG/pki/agent/ -xf "$1"

	echo "Setting up agent configuration..."
	cat >$ICINGA2CONFIG/features-available/agent.conf <<AGENT
/**
 * The agent listener accepts checks from agents.
 */

library "agent"

object AgentListener "agent" {
  cert_path = SysconfDir + "/icinga2/pki/agent/agent.crt"
  key_path = SysconfDir + "/icinga2/pki/agent/agent.key"
  ca_path = SysconfDir + "/icinga2/pki/agent/ca.crt"

  bind_port = 7000
}
AGENT

	echo "Enabling agent feature..."
	@CMAKE_INSTALL_FULL_SBINDIR@/icinga2-enable-feature agent

	echo ""
	echo "The key bundle was installed successfully and the agent component"
	echo "was enabled. Please make sure to restart Icinga 2 for these changes"
	echo "to take effect."
	exit 0
fi

name=$(hostname --fqdn)

echo "Host name: $name"

mkdir -p $ICINGA2CONFIG/pki/agent
chmod 700 $ICINGA2CONFIG/pki
chown @ICINGA2_USER@:@ICINGA2_GROUP@ $ICINGA2CONFIG/pki || exit 1
chmod 700 $ICINGA2CONFIG/pki/agent
chown @ICINGA2_USER@:@ICINGA2_GROUP@ $ICINGA2CONFIG/pki/agent || exit 1

if [ -e $ICINGA2CONFIG/pki/agent/agent.key ]; then
	echo "You already have agent certificates in $ICINGA2CONFIG/pki/agent/"
	exit 1
fi

REQ_COMMON_NAME="$name" KEY_DIR="$ICINGA2CONFIG/pki/agent" openssl req -config $ICINGA2PKIDIR/openssl-quiet.cnf -new -newkey rsa:4096 -keyform PEM -keyout $ICINGA2CONFIG/pki/agent/agent.key -outform PEM -out $ICINGA2CONFIG/pki/agent/agent.csr -nodes && \
	chmod 600 $ICINGA2CONFIG/pki/agent/agent.key

echo "Please sign the following X509 CSR using the Agent CA:"
echo ""

cat $ICINGA2CONFIG/pki/agent/agent.csr

echo ""

echo "You can use the icinga2-sign-key command to sign the CSR. Once signed the"
echo "key bundle can be installed using $0 <bundle>."
exit 0
