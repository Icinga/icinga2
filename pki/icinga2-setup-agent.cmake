#!/bin/sh
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

	while true; do
		echo -n "Do you want this agent instance to listen on a TCP port? [y] "
		if ! read listener; then
			exit 1
		fi

		if [ "$listener" = "y" -o "$listener" = "n" -o -z "$listener" ]; then
			break
		fi

		echo "Please enter 'y' or 'n'."
	done

	if [ -z "$listener" ]; then
		listener=y
	fi

	listener_port=""

	if [ "$listener" = "y" ]; then
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
			while true; do
				echo -n "Master instance IP address/hostname: "
				if ! read upstream_host; then
					exit 1
				fi

				if [ -n "$upstream_host" ]; then
					break
				fi

				echo "Please enter the master instance's hostname."
			done

			while true; do
				echo -n "Master instace port: "
				if ! read upstream_port; then
					exit 1
				fi

				if [ -n "$upstream_port" ]; then
					break
				fi

				echo "Please enter the master instance's port."
			done
		fi
	fi

	echo "Installing the certificate bundle..."
	base64 -d < $1 | tar -C $ICINGA2CONFIG/pki/agent/ -zx || exit 1
	chown @ICINGA2_USER@:@ICINGA2_GROUP@ $ICINGA2CONFIG/pki/agent/* || exit 1

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
AGENT

	if [ "$master" = "n" ]; then
		cat >>$ICINGA2CONFIG/features-available/agent.conf <<AGENT
  upstream_name = "$upstream_name"
AGENT
	fi

	if [ "$listener" = "y" ]; then
		cat >>$ICINGA2CONFIG/features-available/agent.conf <<AGENT
  bind_port = "$listener_port"
AGENT
	fi

	if [ "$upstream_connect" = "y" ]; then
		cat >>$ICINGA2CONFIG/features-available/agent.conf <<AGENT
  upstream_host = "$upstream_host"
  upstream_port = "$upstream_port"
AGENT
	fi

	cat >>$ICINGA2CONFIG/features-available/agent.conf <<AGENT
}
AGENT

	echo "Enabling agent feature..."
	@CMAKE_INSTALL_FULL_SBINDIR@/icinga2-enable-feature agent

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

echo "Please sign the following CSR using the Agent CA:"
echo ""

cat $ICINGA2CONFIG/pki/agent/agent.csr

echo ""

echo "You can use the icinga2-sign-key command to sign the CSR. Once signed the"
echo "key bundle can be installed using $0 <bundle>."
exit 0
