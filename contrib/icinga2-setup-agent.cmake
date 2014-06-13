#!/bin/sh
ICINGA2PKIDIR=@CMAKE_INSTALL_FULL_DATADIR@/icinga2/pki
ICINGA2CONFIG=@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2

name=`hostname --fqdn`

echo "Agent name: $name"

if [ -n "$1" ]; then
	if [ ! -e $ICINGA2CONFIG/pki/$name.key ]; then
		echo "You haven't generated a private key for this Icinga 2 instance"
		echo "yet. Please run this script without any parameters to generate a key."
		exit 1
	fi

	if [ ! -e "$1" ]; then
		echo "The specified key bundle does not exist."
		exit 1
	fi

	if ! base64 -d $1 >/dev/null 2>&1; then
		echo "The bundle file is invalid or corrupted."
		exit 1
	fi

	while true; do
		echo -n "Are you setting up a new master instance? [n] "
		if ! read master; then
			exit 1
		fi

		if [ "$master" = "y" -o "$master" = "n" -o -z "$master" ]; then
			break
		fi

		echo "Please enter 'y' or 'n'."
	done

	if [ -z "$master" ]; then
		master=n
	fi

	upstream_name=""

	if [ "$master" = "n" ]; then
		while true; do
			echo -n "Master Icinga instance name: "
			if ! read upstream_name; then
				exit 1
			fi

			if [ -n "$upstream_name" ]; then
				break
			fi

			echo "Please enter an instance name."
		done
	fi

	listener_port=""

	while true; do
		echo -n "Which TCP port should the agent listen on? [5665] "
		if ! read listener_port; then
			exit 1
		fi

		break
	done

	if [ -z "$listener_port" ]; then
		listener_port=5665
	fi

	upstream_connect=n

	if [ "$master" = "n" ]; then
		while true; do
			echo -n "Do you want this agent instance to connect to the master instance? [y] "
			if ! read upstream_connect; then
				exit 1
			fi

			if [ "$upstream_connect" = "y" -o "$upstream_connect" = "n" -o -z "$upstream_connect" ]; then
				break
			fi

			echo "Please enter 'y' or 'n'."
		done

		if [ -z "$upstream_connect" ]; then
			upstream_connect=y
		fi

		if [ "$upstream_connect" = "y" ]; then
			echo -n "Master instance IP address/hostname [$upstream_name]: "
			if ! read upstream_host; then
				exit 1
			fi

			if [ -z "$upstream_host" ]; then
				upstream_host=$upstream_name
			fi

			echo -n "Master instance port [5665]: "
			if ! read upstream_port; then
				exit 1
			fi

			if [ -z "$upstream_port" ]; then
				upstream_port=5665
			fi
		fi
	fi

	echo "Installing the certificate bundle..."
	base64 -d < $1 | tar -C $ICINGA2CONFIG/pki/ -zx || exit 1
	chown @ICINGA2_USER@:@ICINGA2_GROUP@ $ICINGA2CONFIG/pki/* || exit 1

	echo "Setting up api.conf..."
	cat >$ICINGA2CONFIG/features-available/api.conf <<AGENT
/**
 * The API listener is used for distributed monitoring setups.
 */

object ApiListener "api" {
  cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
  key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
  ca_path = SysconfDir + "/icinga2/pki/ca.crt"

  bind_port = "$listener_port"
}

AGENT

	echo "Setting up zones.conf..."
	cat >$ICINGA2CONFIG/zones.conf <<ZONES
/*
 * Endpoint and Zone configuration for a cluster setup
 * This local example requires `NodeName` defined in
 * constants.conf.
 */

object Endpoint NodeName {
  host = NodeName
}

object Zone ZoneName {
ZONES

	if [ "$upstream_connect" = "y" ]; then
		cat >>$ICINGA2CONFIG/zones.conf <<ZONES
  parent = "$upstream_name"
ZONES
	fi

	cat >>$ICINGA2CONFIG/zones.conf <<ZONES
  endpoints = [ NodeName ]
}

ZONES

	if [ "$upstream_connect" = "y" ]; then
		cat >>$ICINGA2CONFIG/zones.conf <<ZONES
object Endpoint "$upstream_name" {
  host = "$upstream_host"
  port = "$upstream_port"
}

object Zone "$upstream_name" {
  endpoints = [ "$upstream_name" ]
}
ZONES
	fi

	sed -i "s/NodeName = \"localhost\"/NodeName = \"$name\"/" /etc/icinga2/constants.conf

	echo "Enabling API feature..."
	@CMAKE_INSTALL_FULL_SBINDIR@/icinga2-enable-feature api

	if [ ! -e "@CMAKE_INSTALL_FULL_SYSCONFDIR@/monitoring" ]; then
		ln -s $ICINGA2CONFIG/conf.d/hosts/localhost @CMAKE_INSTALL_FULL_SYSCONFDIR@/monitoring
	fi

	if [ "$master" = "n" ]; then
		echo "Disabling notification feature..."
		@CMAKE_INSTALL_FULL_SBINDIR@/icinga2-disable-feature notification
	fi

	echo ""
	echo "The key bundle was installed successfully and the agent component"
	echo "was enabled. Please make sure to restart Icinga 2 for these changes"
	echo "to take effect."
	exit 0
fi

mkdir -p $ICINGA2CONFIG/pki
chmod 700 $ICINGA2CONFIG/pki
chown @ICINGA2_USER@:@ICINGA2_GROUP@ $ICINGA2CONFIG/pki || exit 1

if [ -e $ICINGA2CONFIG/pki/$name.crt ]; then
	echo "You already have agent certificates in $ICINGA2CONFIG/pki/"
	exit 1
fi

REQ_COMMON_NAME="$name" KEY_DIR="$ICINGA2CONFIG/pki/" openssl req -config $ICINGA2PKIDIR/openssl-quiet.cnf -new -newkey rsa:4096 -keyform PEM -keyout $ICINGA2CONFIG/pki/$name.key -outform PEM -out $ICINGA2CONFIG/pki/$name.csr -nodes && \
	chmod 600 $ICINGA2CONFIG/pki/$name.key

echo "Please sign the following CSR using the Agent CA:"
echo ""

cat $ICINGA2CONFIG/pki/$name.csr

echo ""

echo "You can use the icinga2-sign-key command to sign the CSR. Once signed the"
echo "key bundle can be installed using $0 <bundle>."
exit 0
