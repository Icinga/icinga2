#!/bin/sh
#
# This script prepares directories and files needed for running Icinga2
#

# Load sysconf on systems where the initsystem does not pass the environment
if [ "$1" != "" ]; then
	if [ -r "$1" ]; then
		. "$1"
	else
		echo "Unable to read sysconf from '$1'. Exiting." && exit 6
	fi
fi

# Set defaults, to overwrite see "@ICINGA2_SYSCONFIGFILE@"

: ${ICINGA2_USER:="@ICINGA2_USER@"}
: ${ICINGA2_GROUP:="@ICINGA2_GROUP@"}
: ${ICINGA2_COMMAND_GROUP:="@ICINGA2_COMMAND_GROUP@"}
: ${ICINGA2_INIT_RUN_DIR:="@ICINGA2_FULL_INITRUNDIR@"}
: ${ICINGA2_LOG_DIR:="@ICINGA2_FULL_LOGDIR@"}
: ${ICINGA2_CACHE_DIR:="@ICINGA2_FULL_CACHEDIR@"}

getent passwd $ICINGA2_USER >/dev/null 2>&1 || (echo "Icinga user '$ICINGA2_USER' does not exist. Exiting." && exit 6)
getent group $ICINGA2_GROUP >/dev/null 2>&1 || (echo "Icinga group '$ICINGA2_GROUP' does not exist. Exiting." && exit 6)
getent group $ICINGA2_COMMAND_GROUP >/dev/null 2>&1 || (echo "Icinga command group '$ICINGA2_COMMAND_GROUP' does not exist. Exiting." && exit 6)

if [ ! -e "$ICINGA2_INIT_RUN_DIR" ]; then
	mkdir "$ICINGA2_INIT_RUN_DIR"
	mkdir "$ICINGA2_INIT_RUN_DIR"/cmd
fi

chmod 755 "$ICINGA2_INIT_RUN_DIR"
chmod 2750 "$ICINGA2_INIT_RUN_DIR"/cmd
chown -R $ICINGA2_USER:$ICINGA2_COMMAND_GROUP "$ICINGA2_INIT_RUN_DIR"

test -e "$ICINGA2_LOG_DIR" || install -m 750 -o $ICINGA2_USER -g $ICINGA2_COMMAND_GROUP -d "$ICINGA2_LOG_DIR"

if type restorecon >/dev/null 2>&1; then
	restorecon -R "$ICINGA2_INIT_RUN_DIR"/
fi

test -e "$ICINGA2_CACHE_DIR" || install -m 750 -o $ICINGA2_USER -g $ICINGA2_COMMAND_GROUP -d "$ICINGA2_CACHE_DIR"
