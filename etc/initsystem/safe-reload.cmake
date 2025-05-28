#!/bin/sh

# Load sysconf on systems where the initsystem does not pass the environment
if [ "$1" != "" ]; then
	if [ -r "$1" ]; then
		. "$1"
	else
		echo "Unable to read sysconf from '$1'. Exiting."
		exit 6
	fi
fi

# Set defaults, to overwrite see "@ICINGA2_SYSCONFIGFILE@"

: "${ICINGA2_PID_FILE:="@ICINGA2_FULL_INITRUNDIR@/icinga2.pid"}"
: "${DAEMON:="@CMAKE_INSTALL_FULL_SBINDIR@/icinga2"}"

printf "Validating config files: "

OUTPUTFILE=`mktemp`

if type selinuxenabled >/dev/null 2>&1; then
	if selinuxenabled; then
		chcon -t icinga2_tmp_t "$OUTPUTFILE" >/dev/null 2>&1
	fi
fi

: "${FIRST_ARG:="@ICINGA2_SYSCONFIGFILE@"}"

ICINGA2_ARGS=""

for arg in "$@"; do
	if [ "$arg" = "$FIRST_ARG" ]; then
		continue;
	fi

	ICINGA2_ARGS=$(printf '%s%s' "$ICINGA2_ARGS" "$arg")
done

if ! "$DAEMON" daemon --validate --color "$ICINGA2_ARGS" > "$OUTPUTFILE"; then
	echo "Failed"

	cat "$OUTPUTFILE"
	rm -f "$OUTPUTFILE"
	exit 1
fi

echo "Done"
rm -f "$OUTPUTFILE"

printf "Reloading Icinga 2: "

if [ ! -e "$ICINGA2_PID_FILE" ]; then
	exit 7
fi

pid=`cat "$ICINGA2_PID_FILE"`
if ! kill -HUP "$pid" >/dev/null 2>&1; then
	echo "Error: Icinga not running"
	exit 7
fi

echo "Done"
exit 0
