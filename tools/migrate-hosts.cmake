#!/bin/sh
# Moves configuration files from /etc/icinga2/conf.d/hosts/<fqdn>
# as backup files. TODO: Remove script before 2.2 release.

icinga2bin=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2
sysconfdir=`$icinga2bin variable get --current SysconfDir`

if [ -z "$sysconfdir" ]; then
    	echo "Could not determine SysconfDir"
    	exit 1
fi

target="`hostname -f`"
if [ ! -e $sysconfdir/icinga2/repository.d/hosts/$target.conf ]; then
    	exit 0
fi

mv $sysconfdir/icinga2/repository.d/hosts/$target.conf $sysconfdir/icinga2/repository.d/hosts/$target.conf.bak

if [ -d $sysconfdir/icinga2/repository.d/hosts/$target ]; then
	for file in $sysconfdir/icinga2/repository.d/hosts/$target/*.conf; do
        	if [ ! -e $file ]; then
           		break
                fi

		mv $file $file.bak
	done
fi

echo "Moved repository FQDN host in $sysconfdir/icinga2/repository.d/hosts/$target as backup. Please migrate your changes to all new conf.d/{hosts,services.conf}"
exit 0
