#!/bin/sh
ICINGA2CONFDIR=@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2

TOOL=$(basename -- $0)

if [ "$TOOL" != "icinga2-enable-feature" -a "$TOOL" != "icinga2-disable-feature" ]; then
	echo "Invalid tool name ($TOOL). Should be 'icinga2-enable-feature' or 'icinga2-disable-feature'."
	exit 1
fi

if [ -z "$1" ]; then
	echo "Syntax: $0 <feature>"

	if [ "$TOOL" = "icinga2-enable-feature" ]; then
		echo "Enables the specified feature."
	else
		echo "Disables the specified feature."
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

FEATURE=$1

if [ ! -e $ICINGA2CONFDIR/features-available/$FEATURE.conf ]; then
	echo "The feature '$FEATURE' does not exist."
	exit 1
fi

if [ "$TOOL" = "icinga2-enable-feature" ]; then
	if [ -e $ICINGA2CONFDIR/features-enabled/$FEATURE.conf ]; then
		echo "The feature '$FEATURE' is already enabled."
		exit 0
	fi

	if ! ln -s ../features-available/$FEATURE.conf $ICINGA2CONFDIR/features-enabled/; then
	echo "Enabling '$FEATURE' failed. Check permissions for $ICINGA2CONFDIR/features-enabled/"
		exit 1
	else
		echo "Module '$FEATURE' was enabled."
	fi
elif [ "$TOOL" = "icinga2-disable-feature" ]; then
	if [ ! -e $ICINGA2CONFDIR/features-enabled/$FEATURE.conf ]; then
		echo "The feature '$FEATURE' is already disabled."
		exit 0
	fi

	if ! rm -f $ICINGA2CONFDIR/features-enabled/$FEATURE.conf; then
	echo "Disabling '$FEATURE' failed. Check permissions for $ICINGA2CONFDIR/features-enabled/$FEATURE.conf"
		exit 1
	else
		echo "Module '$FEATURE' was disabled."
	fi
fi

echo "Make sure to restart Icinga 2 for these changes to take effect."
exit 0
