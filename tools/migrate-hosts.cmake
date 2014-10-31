#!/bin/sh
# Moves configuration files from /etc/icinga2/conf.d/hosts
# to /etc/icinga2/repository.d

icinga2bin=@CMAKE_INSTALL_FULL_SBINDIR@/icinga2
sysconfdir=`$icinga2bin variable get --current SysconfDir`

if [ -z "$sysconfdir" ]; then
    echo "Could not determine SysconfDir"
    exit 1
fi

if [ ! -d $sysconfdir/icinga2/conf.d/hosts ]; then
    exit 0
fi

mkdir -p $sysconfdir/icinga2/repository.d/hosts

host_count=0
service_count=0

for hostFile in $sysconfdir/icinga2/conf.d/hosts/*.conf; do
    if [ ! -e $hostFile ]; then
        continue
    fi

    host_count=$(($host_count + 1))

    host=`basename $hostFile .conf`

    if [ "x$host" = "xlocalhost" ]; then
        target="`hostname -f`"
    else
        target=$host
    fi

    if [ ! -e $sysconfdir/icinga2/repository.d/hosts/$target.conf ]; then
        mv $sysconfdir/icinga2/conf.d/hosts/$host.conf $sysconfdir/icinga2/repository.d/hosts/$target.conf
        sed "s/localhost/$target/g" $sysconfdir/icinga2/repository.d/hosts/$target.conf > $sysconfdir/icinga2/repository.d/hosts/$target.conf.tmp
        mv $sysconfdir/icinga2/repository.d/hosts/$target.conf.tmp $sysconfdir/icinga2/repository.d/hosts/$target.conf
    else
        rm -f $sysconfdir/icinga2/conf.d/hosts/$host.conf
    fi

    if [ -d $sysconfdir/icinga2/conf.d/hosts/$host ]; then
        service_count=$(($service_count + 1))

        if [ ! -e $sysconfdir/icinga2/repository.d/hosts/$target ]; then
            mv $sysconfdir/icinga2/conf.d/hosts/$host $sysconfdir/icinga2/repository.d/hosts/$target
            for file in $sysconfdir/icinga2/repository.d/hosts/$target/*.conf; do
                if [ ! -e $file ]; then
                    break
                fi

                sed "s/localhost/$target/g" $file > $file.tmp
                mv $file.tmp $file
            done
        else
            rm -rf $sysconfdir/icinga2/conf.d/hosts/$host
        fi
    fi
done

cat >$sysconfdir/icinga2/conf.d/hosts/README <<TEXT
What happened to my configuration files?
========================================

Your host and service configuration files were moved to the $sysconfdir/icinga2/repository.d directory.

This allows you to manipulate those files using the "icinga2 repository" CLI commands.

Here are a few commands you might want to try:

# icinga2 repository host list

# icinga2 repository service list

# icinga2 repository --help
TEXT

echo "Migrated $host_count host(s) and $service_count service(s)."

exit 0
