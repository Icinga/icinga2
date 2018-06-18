#!/bin/sh
#
# This script prepares directories and files needed for running Icinga2
#

# Load sysconf on systems where the initsystem does not pass the environment
if [ "$1" != "" ]; then
	if [ -r "$1" ]; then
		source "$1"
	else
		echo "Unable to read sysconf from '$1'. Exiting." && exit 6
	fi
fi

# Set defaults, to overwrite see "@ICINGA2_SYSCONFIGFILE@"

: ${ICINGA2_USER:="@ICINGA2_USER@"}
: ${ICINGA2_GROUP:="@ICINGA2_GROUP@"}
: ${ICINGA2_COMMAND_GROUP:="@ICINGA2_COMMAND_GROUP@"}
: ${ICINGA2_RUN_DIR:="@ICINGA2_RUNDIR@"}
: ${ICINGA2_LOG_DIR:="@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/log/icinga2"}
: ${ICINGA2_STATE_DIR:="@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/cache/icinga2"}
: ${ICINGA2_CACHE_DIR:="@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/cache/icinga2"}

getent passwd $ICINGA2_USER >/dev/null 2>&1 || (echo "Icinga user '$ICINGA2_USER' does not exist. Exiting." && exit 6)
getent group $ICINGA2_GROUP >/dev/null 2>&1 || (echo "Icinga group '$ICINGA2_GROUP' does not exist. Exiting." && exit 6)
getent group $ICINGA2_COMMAND_GROUP >/dev/null 2>&1 || (echo "Icinga command group '$ICINGA2_COMMAND_GROUP' does not exist. Exiting." && exit 6)

if [ ! -e "$ICINGA2_RUN_DIR"/icinga2 ]; then
	mkdir "$ICINGA2_RUN_DIR"/icinga2
	mkdir "$ICINGA2_RUN_DIR"/icinga2/cmd
fi

chmod 755 "$ICINGA2_RUN_DIR"/icinga2
chmod 2750 "$ICINGA2_RUN_DIR"/icinga2/cmd
chown -R $ICINGA2_USER:$ICINGA2_COMMAND_GROUP "$ICINGA2_RUN_DIR"/icinga2

test -e "$ICINGA2_LOG_DIR" || install -m 750 -o $ICINGA2_USER -g $ICINGA2_COMMAND_GROUP -d "$ICINGA2_LOG_DIR"

if type restorecon >/dev/null 2>&1; then
	restorecon -R "$ICINGA2_RUN_DIR"/icinga2/
fi

test -e "$ICINGA2_CACHE_DIR" || install -m 750 -o $ICINGA2_USER -g $ICINGA2_COMMAND_GROUP -d "$ICINGA2_CACHE_DIR"
