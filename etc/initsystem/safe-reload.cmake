#!/bin/sh

: ${ICINGA2_PID_FILE:="@ICINGA2_RUNDIR@/icinga2/icinga2.pid"}
: ${DAEMON:="@CMAKE_INSTALL_FULL_SBINDIR@/icinga2"}

printf "Validating config files: "

OUTPUTFILE=`mktemp`

if type selinuxenabled >/dev/null 2>&1; then
	if selinuxenabled; then
		chcon -t icinga2_tmp_t $OUTPUTFILE >/dev/null 2>&1
	fi
fi

if ! $DAEMON daemon --validate --color > $OUTPUTFILE; then
	echo "Failed"

	cat $OUTPUTFILE
	rm -f $OUTPUTFILE
	exit 1
fi

echo "Done"
rm -f $OUTPUTFILE

printf "Reloading Icinga 2: "

if [ ! -e $ICINGA2_PID_FILE ]; then
	exit 7
fi

pid=`cat $ICINGA2_PID_FILE`
if ! kill -HUP $pid >/dev/null 2>&1; then
	echo "Error: Icinga not running"
	exit 7
fi

echo "Done"
exit 0
