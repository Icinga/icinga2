#!/bin/sh -e

DIRNAME=`dirname $0`
cd $DIRNAME
USAGE="$0 [ --update ]"
if [ `id -u` != 0 ]; then
echo 'You must be root to run this script'
exit 1
fi

if [ $# -eq 1 ]; then
	if [ "$1" = "--update" ] ; then
		time=`ls -l --time-style="+%x %X" icinga2.te | awk '{ printf "%s %s", $6, $7 }'`
		rules=`ausearch --start $time -m avc --raw -se icinga2`
		if [ x"$rules" != "x" ] ; then
			echo "Found avc's to update policy with"
			echo -e "$rules" | audit2allow -R
			echo "Do you want these changes added to policy [y/n]?"
			read ANS
			if [ "$ANS" = "y" -o "$ANS" = "Y" ] ; then
				echo "Updating policy"
				echo -e "$rules" | audit2allow -R >> icinga2.te
				# Fall though and rebuild policy
			else
				exit 0
			fi
		else
			echo "No new avcs found"
			exit 0
		fi
	else
		echo -e $USAGE
		exit 1
	fi
elif [ $# -ge 2 ] ; then
	echo -e $USAGE
	exit 1
fi

echo "Building and Loading Policy"
set -x
make -f /usr/share/selinux/devel/Makefile icinga2.pp || exit
/usr/sbin/semodule -i icinga2.pp

# Generate a man page off the installed module
sepolicy manpage -p . -d icinga2_t
# Fixing the file context on /usr/sbin/icinga2
/sbin/restorecon -F -R -v /usr/sbin/icinga2
# Fixing the file context on /etc/rc\.d/init\.d/icinga2
#/sbin/restorecon -F -R -v /etc/rc\.d/init\.d/icinga2
# Fixing the file context on /etc/icinga2/scripts
/sbin/restorecon -F -R -v /etc/icinga2/scripts
# Fixing the file context on /var/log/icinga2
/sbin/restorecon -F -R -v /var/log/icinga2
# Fixing the file context on /var/lib/icinga2
/sbin/restorecon -F -R -v /var/lib/icinga2
# Fixing the file context on /var/run/icinga2
/sbin/restorecon -F -R -v /var/run/icinga2
# Fixing the file context on /var/cache/icinga2
/sbin/restorecon -F -R -v /var/cache/icinga2
# Fixing the file context on /var/spool/icinga2
/sbin/restorecon -F -R -v /var/spool/icinga2

# Fix dir permissions until we have it in the package
chown root /etc/icinga2
chown root /etc/icinga2/init.conf

# Label the port 5665
/sbin/semanage port -a -t icinga2_port_t -p tcp 5665

# Generate a rpm package for the newly generated policy
pwd=$(pwd)
#rpmbuild --define "_sourcedir ${pwd}" --define "_specdir ${pwd}" --define "_builddir ${pwd}" --define "_srcrpmdir ${pwd}" --define "_rpmdir ${pwd}" --define "_buildrootdir ${pwd}/.build"  -ba icinga2_selinux.spec
