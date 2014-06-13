#!/bin/sh
ICINGA2CONFDIR=@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2

TOOL=$(basename -- $0)

if [ "$TOOL" != "icinga2-enable-feature" -a "$TOOL" != "icinga2-disable-feature" ]; then
	echo "Invalid tool name ($TOOL). Should be 'icinga2-enable-feature' or 'icinga2-disable-feature'."
	exit 1
fi

if [ -z "$1" ]; then
	echo "Syntax: $TOOL <features separated with whitespaces>"
	echo "  Example: $TOOL checker notification mainlog"

	if [ "$TOOL" = "icinga2-enable-feature" ]; then
		echo "Enables the specified feature(s)."
	else
		echo "Disables the specified feature(s)."
	fi

	echo
	echo -n "Available features: "

	for file in $ICINGA2CONFDIR/features-available/*.conf; do
		echo -n "$(basename -- $file .conf) "
	done

	echo
	echo -n "Enabled features: "

	for file in $ICINGA2CONFDIR/features-enabled/*.conf; do
		echo -n "$(basename -- $file .conf) "
	done

	echo

	exit 1
fi

FEATURES=$1

for FEATURES
do
	SKIP=""
	# Define array var
	# Based http://blog.isonoe.net/post/2010/09/24/Pseudo-arrays-for-POSIX-shell
	eval "set -- $FEATURES"
	for FEATURE
	do
		SKIP="NOTOK"
		if [ ! -e $ICINGA2CONFDIR/features-available/$FEATURE.conf ]; then
			echo "Feature '$FEATURE' does not exist."
			exit 1
		fi

		if [ "$TOOL" = "icinga2-enable-feature" ]; then
			if [ -e $ICINGA2CONFDIR/features-enabled/$FEATURE.conf ]; then
				echo "The feature '$FEATURE' is already enabled."
				SKIP="OK"
			fi
			if [ "$SKIP" != "OK" ]; then
				if ! ln -s ../features-available/$FEATURE.conf $ICINGA2CONFDIR/features-enabled/; then
				echo "Enabling '$FEATURE' failed. Check permissions for $ICINGA2CONFDIR/features-enabled/"
					exit 1
				else
					echo "Module '$FEATURE' was enabled."
					RELOAD="YES"
				fi
			fi
		elif [ "$TOOL" = "icinga2-disable-feature" ]; then
			if [ ! -e $ICINGA2CONFDIR/features-enabled/$FEATURE.conf ]; then
				echo "The feature '$FEATURE' is already disabled."
				SKIP="OK"
			fi

			if [ "$SKIP" != "OK" ]; then
				if ! rm -f $ICINGA2CONFDIR/features-enabled/$FEATURE.conf; then
				echo "Disabling '$FEATURE' failed. Check permissions for $ICINGA2CONFDIR/features-enabled/$FEATURE.conf"
					exit 1
				else
					echo "Feature '$FEATURE' was disabled."
					RELOAD="YES"
				fi
			fi
		fi
	done
done
if [ "$RELOAD" = "YES" ]; then
	echo "Make sure to restart Icinga 2 for these changes to take effect."
fi
exit 0
